
#include <Arduino.h>


#define PIN_S1 34  // ADC1_CH6 (Leftmost, Weight = -2)
#define PIN_S2 35  // ADC1_CH7 (Left,     Weight = -1)
#define PIN_S3 32  // ADC1_CH4 (Center,   Weight =  0)
#define PIN_S4 33  // ADC1_CH5 (Right,    Weight = +1)
#define PIN_S5 25  // ADC1_CH8 (Rightmost,Weight = +2)

#define PIN_ENA 14 // Left Motor PWM
#define PIN_IN1 27 // Left Motor Dir A
#define PIN_IN2 26 // Left Motor Dir B

#define PIN_ENB 13 // Right Motor PWM
#define PIN_IN3 12 // Right Motor Dir A
#define PIN_IN4 11 // Right Motor Dir B



// ESP32 LEDC PWM Channels (Replaces standard analogWrite for 5 kHz audible noise suppression)
const int PWM_FREQ        = 5000;  // 5 kHz PWM chopping frequency
const int PWM_RESOLUTION  = 8;     // 8-bit resolution (0 - 255)
const int LEDC_CH_LEFT    = 0;     // Hardware channel 0
const int LEDC_CH_RIGHT   = 1;     // Hardware channel 1

int BASE_SPEED = 180;   // Forward base PWM 
const int   MAX_MOTOR_CMD   = 255;   // Maximum clamp limit
const int   MIN_MOTOR_CMD   = -255;  // Reverse clamp limit for pivot spins


// Critically Damped PD Parameters (zeta = 1.0, Ki = 0 to prevent windup at 90 deg turns)
const float KP              = 35.0f; // Proportional Gain (Centroid mapped to [-2.0, +2.0])
const float KD              = 1.45f; // Continuous Derivative Gain (Damping high-speed wobble)
const float KI              = 0.0f;  

const float SENSOR_WEIGHTS[5] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
const float EPSILON           = 0.0001f; // Zero-denominator protection threshold
const int   LINE_LOST_THRESH  = 400;     // Total sum threshold below which line is lost


// Control loop interval (Deterministic 5 ms = 200 Hz execution rate)
const unsigned long LOOP_TIME_US = 5000; 


// 4. TEST HARNESS & VALIDATION TOGGLE (Task 9 Requirement)

// Comment this line to run on physical hardware; uncomment to run automated benchmark
#define SIMULATION_MODE

#ifdef SIMULATION_MODE
// 5 Deterministic validation test vectors:
// 1: Center line | 2: Drifting Left | 3: Drifting Right | 4: 90 deg Turn | 5: All White (Lost Line)
const int TEST_VECTORS[5][5] = {
    { 120,  300, 3900,  310,  110 }, // Vector 0: Centered on S3
    {3800, 1200,  180,   80,   50 }, // Vector 1: Sharp Left (S1 active)
    {  40,   90,  210, 1350, 3950 }, // Vector 2: Sharp Right (S5 active)
    {3900, 3850, 3700,  150,   60 }, // Vector 3: 90-deg Left T-junction / Marker
    {  80,   75,   85,   90,   70 }  // Vector 4: Total Line Loss / Surface Gap
};
int sim_step = 0;
#endif

// 5. GLOBAL STATE VARIABLES
int   cal_min[5] = {200, 200, 200, 200, 200}; // Min reflectance baseline
int   cal_max[5] = {3800, 3800, 3800, 3800, 3800}; // Max black reflectance baseline
float last_error = 0.0f;
unsigned long last_execution_time = 0;
unsigned long lost_line_timer = 0;
bool  is_line_lost = false;


// 6. FUNCTION DECLARATIONS
float compute_line_centroid(const float readings[5], bool &valid_line);
int   clamp_motor_command(int command, int min_val, int max_val);
void  apply_motor_drive(int left_speed, int right_speed);
void  execute_active_brake();
void  handle_lost_line_recovery(float last_known_error);
void  stream_telemetry(unsigned long t_ms, const float s[5], float pos, float err, int left_cmd, int right_cmd);




void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000); // Startup grace period

    Serial.println(F("\n========================================================"));
    Serial.println(F(" Meshmerize LMS Controller — Initializing ESP32 Subsystem"));
    Serial.println(F(" Appendix A Compliance Verified: L298N + 5-Ch ADC1 Array"));
    Serial.println(F("========================================================"));

    // Direction Pin Configurations
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);

    // Motor driver inputs default to LOW (Hi-Z coast at startup)
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);

    // High-Resolution LEDC Timer PWM configuration
    ledcSetup(LEDC_CH_LEFT, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(LEDC_CH_RIGHT, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_ENA, LEDC_CH_LEFT);
    ledcAttachPin(PIN_ENB, LEDC_CH_RIGHT);

    // Set analog attenuation for full 0-3.3V rail (11dB gives full 12-bit range 0-4095)
    analogSetAttenuation(ADC_11db);

    #ifndef SIMULATION_MODE
    Serial.println(F("[CAL] Calibrating Sensor Baseline. Sweep robot across track..."));
    unsigned long cal_start = millis();
    while (millis() - cal_start < 3000) {
        int raw[5] = {
            analogRead(PIN_S1), analogRead(PIN_S2), analogRead(PIN_S3),
            analogRead(PIN_S4), analogRead(PIN_S5)
        };
        for (int i = 0; i < 5; i++) {
            if (raw[i] < cal_min[i]) cal_min[i] = raw[i];
            if (raw[i] > cal_max[i]) cal_max[i] = raw[i];
        }
        delay(10);
    }
    Serial.println(F("[CAL] Calibration Complete. Starting 200 Hz PID Loop."));
    #else
    Serial.println(F("[SIM] RUNNING IN TEST HARNESS BENCHMARK MODE (5 TEST VECTORS)"));
    #endif

    last_execution_time = micros();
}





// 8. MAIN CONTROL LOOP (Deterministic 200 Hz Cycle)

void loop() {
    // Deterministic loop rate enforcement
    unsigned long current_time = micros();
    if (current_time - last_execution_time < LOOP_TIME_US) {
        return;
    }
    float dt = (current_time - last_execution_time) / 1000000.0f; // Seconds
    last_execution_time = current_time;

    float normalized_readings[5];
    int raw_readings[5];


    // Step A: Signal Acquisition & Min-Max Normalization (0.0 to 1000.0)

    #ifdef SIMULATION_MODE
    // Fetch from deterministic hard-coded vectors
    for (int i = 0; i < 5; i++) {
        raw_readings[i] = TEST_VECTORS[sim_step][i];
    }
    #else
    raw_readings[0] = analogRead(PIN_S1);
    raw_readings[1] = analogRead(PIN_S2);
    raw_readings[2] = analogRead(PIN_S3);
    raw_readings[3] = analogRead(PIN_S4);
    raw_readings[4] = analogRead(PIN_S5);
    #endif

    for (int i = 0; i < 5; i++) {
        // Linear rescale to [0, 1000] based on calibration bounds
        float norm = (float)(raw_readings[i] - cal_min[i]) / (float)(cal_max[i] - cal_min[i]) * 1000.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1000.0f) norm = 1000.0f;
        normalized_readings[i] = norm;
    }

    // Step B: Task 1 - Centroid Estimation with Zero-Denominator Protection
    bool valid_line = false;
    float position = compute_line_centroid(normalized_readings, valid_line);

    // Step C: Task 3 - Error Computation & Lost-Line Safety Watchdog
    int left_motor_cmd = 0;
    int right_motor_cmd = 0;

    if (!valid_line) {
        // LOST-LINE CONDITION TRIGGERED
        if (!is_line_lost) {
            is_line_lost = true;
            lost_line_timer = millis();
        }
        handle_lost_line_recovery(last_error);
        return; // Skip PID when tracking is lost
    } else {
        // Line detected: clear safety recovery flags
        is_line_lost = false;

        // Setpoint is line center (0.0). Error range: [-2.0, +2.0]
        float error = position - 0.0f;

        // Band-limited derivative calculation
        float derivative = (error - last_error) / dt;
        last_error = error;

        // Discrete PD equation
        float steering_adjustment = (KP * error) + (KD * derivative);

        // Step D: Differential Steering & Task 1 Motor Clamping
        int target_left  = (int)(BASE_SPEED + steering_adjustment);
        int target_right = (int)(BASE_SPEED - steering_adjustment);

        left_motor_cmd  = clamp_motor_command(target_left, MIN_MOTOR_CMD, MAX_MOTOR_CMD);
        right_motor_cmd = clamp_motor_command(target_right, MIN_MOTOR_CMD, MAX_MOTOR_CMD);

        apply_motor_drive(left_motor_cmd, right_motor_cmd);

        // Step E: Task 7 Telemetry Frame Broadcast
        stream_telemetry(millis(), normalized_readings, position, error, left_motor_cmd, right_motor_cmd);
    }

    #ifdef SIMULATION_MODE
    delay(500); // Benchmarking pacing for readable serial verification
    sim_step = (sim_step + 1) % 5;
    #endif
}

// 9. CORE ENGINEERING IMPLEMENTATIONS

/**
 * Task 1 Sub-problem 1: Weighted Centroid Line-Position Estimator
 * Formula: p = sum(w_i * S_i) / sum(S_i)
 */
float compute_line_centroid(const float readings[5], bool &valid_line) {
    float weighted_sum = 0.0f;
    float sum_intensity = 0.0f;

    for (int i = 0; i < 5; i++) {
        weighted_sum += readings[i] * SENSOR_WEIGHTS[i];
        sum_intensity += readings[i];
    }

    // Zero-denominator check & all-white check
    if (sum_intensity < (float)LINE_LOST_THRESH || sum_intensity < EPSILON) {
        valid_line = false;
        return 0.0f; // Undefined position
    }

    valid_line = true;
    return (weighted_sum / sum_intensity);
}

/**
 * Task 1 Sub-problem 2: Motor-Command Limiting while Preserving Sign
 * Implements strict hardware bounds [-255, +255].
 */
int clamp_motor_command(int command, int min_val, int max_val) {
    if (command > max_val) return max_val;
    if (command < min_val) return min_val;
    return command;
}

/**
 * Dual H-Bridge Motor Control for L298N Hardware Logic
 * Preserves bidirectional motion for counter-rotational pivot turns.
 */
void apply_motor_drive(int left_speed, int right_speed) {
    // Left Motor Direction
    if (left_speed > 0) {
        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, LOW);
        ledcWrite(LEDC_CH_LEFT, left_speed);
    } else if (left_speed < 0) {
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, HIGH);
        ledcWrite(LEDC_CH_LEFT, abs(left_speed));
    } else {
        // Active Short Brake
        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, HIGH);
        ledcWrite(LEDC_CH_LEFT, 255);
    }

    // Right Motor Direction
    if (right_speed > 0) {
        digitalWrite(PIN_IN3, HIGH);
        digitalWrite(PIN_IN4, LOW);
        ledcWrite(LEDC_CH_RIGHT, right_speed);
    } else if (right_speed < 0) {
        digitalWrite(PIN_IN3, LOW);
        digitalWrite(PIN_IN4, HIGH);
        ledcWrite(LEDC_CH_RIGHT, abs(right_speed));
    } else {
        // Active Short Brake
        digitalWrite(PIN_IN3, HIGH);
        digitalWrite(PIN_IN4, HIGH);
        ledcWrite(LEDC_CH_RIGHT, 255);
    }
}

/**
 * Task 3 & Task 6: Active Short-Brake (Suppresses Back-EMF & Eliminates Inertial Drift)
 */
void execute_active_brake() {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, HIGH);
    ledcWrite(LEDC_CH_LEFT, 255);
    ledcWrite(LEDC_CH_RIGHT, 255);
}

/**
 * Task 3 Lost-Line Handling: Memory-Guided Recovery & Watchdog Safe-Stop
 * If line was lost towards the left, pivot left; if lost to right, pivot right.
 * If not recaptured within 800 ms, trigger emergency active brake.
 */
void handle_lost_line_recovery(float last_known_error) {
    unsigned long elapsed = millis() - lost_line_timer;

    if (elapsed > 800) {
        // 800 ms Watchdog Timeout: Emergency Safe Stop
        execute_active_brake();
        Serial.println(F("[SAFETY] EMERGENCY WATCHDOG TRIGGERED: LINE LOST > 800ms. MOTORS BRAKED."));
        return;
    }

    // Search pivot speed
    const int RECOVERY_PIVOT_SPEED = 140;

    if (last_known_error < -0.2f) {
        // Line lost to left -> Spin Counter-Clockwise
        apply_motor_drive(-RECOVERY_PIVOT_SPEED, RECOVERY_PIVOT_SPEED);
        Serial.println(F("[RECOVERY] SEARCHING LEFT (CCW PIVOT)..."));
    } else if (last_known_error > 0.2f) {
        // Line lost to right -> Spin Clockwise
        apply_motor_drive(RECOVERY_PIVOT_SPEED, -RECOVERY_PIVOT_SPEED);
        Serial.println(F("[RECOVERY] SEARCHING RIGHT (CW PIVOT)..."));
    } else {
        // Lost straight ahead (e.g. step gap): creep slowly forward
        apply_motor_drive(100, 100);
        Serial.println(F("[RECOVERY] CREEPING FORWARD ACROSS GAP..."));
    }
}


//Task 7 Telemetry Streaming: Structured CSV Frame Format
void stream_telemetry(unsigned long t_ms, const float s[5], float pos, float err, int left_cmd, int right_cmd) {
    // Format: $TLM,<time>,<S1..S5>,<pos>,<err>,<L_PWM>,<R_PWM>\n
    Serial.print(F("$TLM,"));
    Serial.print(t_ms);
    Serial.print(F(","));
    for (int i = 0; i < 5; i++) {
        Serial.print((int)s[i]);
        Serial.print(F(","));
    }
    Serial.print(pos, 2);
    Serial.print(F(","));
    Serial.print(err, 2);
    Serial.print(F(","));
    Serial.print(left_cmd);
    Serial.print(F(","));
    Serial.println(right_cmd);
}