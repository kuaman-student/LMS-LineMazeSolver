#include <Arduino.h>


const int SENSOR_PINS[5] = {34, 35, 32, 33, 25};

// Toshiba TB6612FNG Motor Driver Pins
const int PIN_PWMA = 14;
const int PIN_AIN1 = 27;
const int PIN_AIN2 = 26;
const int PIN_PWMB = 13;
const int PIN_BIN1 = 12;
const int PIN_BIN2 = 11;

// Coordinate Weights & Tuned PD Parameters
const float WEIGHTS[5] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};

// PD Controller Gains (Calibrated for V_base = 180)
const float KP = 48.0f;
const float KD = 24.0f;

// Exponential Moving Average (EMA) Smoothing Factor for D-term
const float ALPHA = 0.70f;

// Speed Bounds
const int BASE_SPEED   = 180;
const int MAX_PWM      = 255;
const int MIN_PWM      = -255;
const int SEARCH_SPEED = 140;

// Thresholds for Edge Cases (Normalized 0 - 1000 scale)
const float EPSILON_SUM            = 50.0f;   // Hard math guard against division by zero
const float LOST_LINE_SUM          = 150.0f;  // Case 1: All sensors see white (Line Lost / Gap)
const float ALL_BLACK_SUM          = 4200.0f; // Case 2: All sensors see black (T-Point / Cross)
const unsigned long LOST_TIMEOUT_MS = 1200;    // 1.2s watchdog safety stop

// State Machine Variables
float previous_error       = 0.0f;
float last_valid_error     = 0.0f;     // Remembers which direction line was lost
float prev_d_filtered      = 0.0f;
unsigned long last_loop_time_us = 0;
unsigned long line_lost_start_ms = 0;
bool is_line_lost          = false;

// Function Prototypes
void set_motor_output(int left_pwm, int right_pwm);
void apply_active_brake();

void setup() {
    Serial.begin(115200);

    pinMode(PIN_PWMA, OUTPUT);
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_PWMB, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);

    // Initial state: 10ms short-brake to clear back-EMF & stabilize
    apply_active_brake();
    delay(10);

    last_loop_time_us = micros();
}

void loop() {
    // 1. Deterministic Loop Timing (200 Hz -> dt = 5 ms)
    unsigned long current_time_us = micros();
    float dt = (current_time_us - last_loop_time_us) / 1000000.0f;
    if (dt < 0.005f) {
        return;
    }
    last_loop_time_us = current_time_us;

    // 2. Sensor Acquisition & Normalization
    float weighted_sum = 0.0f;
    float total_sum    = 0.0f;
    int sensor_raw[5];

    for (int i = 0; i < 5; i++) {
        // Map 12-bit ADC (0-4095) to calibrated reflectance (0-1000)
        sensor_raw[i] = map(analogRead(SENSOR_PINS[i]), 0, 4095, 0, 1000);
        sensor_raw[i] = constrain(sensor_raw[i], 0, 1000);

        weighted_sum += (float)sensor_raw[i] * WEIGHTS[i];
        total_sum    += (float)sensor_raw[i];
    }

    float current_error = 0.0f;

    // 3. TASK 1 CRITICAL EDGE CASES HANDLING
    // --- EDGE CASE 1: Full Line Loss / Division-by-Zero Guard ---
    if (total_sum < LOST_LINE_SUM || total_sum <= EPSILON_SUM) {
        if (!is_line_lost) {
            is_line_lost = true;
            line_lost_start_ms = millis();
        }

        // Safety Watchdog: Complete halt if track not recovered within timeout
        if (millis() - line_lost_start_ms > LOST_TIMEOUT_MS) {
            apply_active_brake();
            return;
        }

        // Latch to the last valid direction instead of computing 0/0
        if (last_valid_error >= 0.0f) {
            // Line slipped to the right -> Spin clockwise on axis
            set_motor_output(SEARCH_SPEED, -SEARCH_SPEED);
        } else {
            // Line slipped to the left -> Spin counter-clockwise on axis
            set_motor_output(-SEARCH_SPEED, SEARCH_SPEED);
        }
        return;
    }

    // --- EDGE CASE 2: All-Black Grid / T-Junction / Cross ---
    if (total_sum >= ALL_BLACK_SUM) {
        // When all sensors hit black, mathematical centroid evaluates to ~0.0
        // Preventing straightfalse " cruising" behavior:
        is_line_lost = false;
        
        // Execute Junction Hold: Maintain stable forward inching or FSM Handover
        // Here we coast forward smoothly to allow maze solver decision
        set_motor_output(140, 140);
        return;
    }

    // Line is reliably detected -> Clear lost flags
    is_line_lost = false;

    // --- EDGE CASE 3: Safe Division & Bound Protection ---
    current_error = weighted_sum / total_sum;
    current_error = constrain(current_error, -2.0f, +2.0f);

    // Update memory of last known solid tracking side
    if (fabs(current_error) > 0.2f) {
        last_valid_error = current_error;
    }

    // 4. Derivative with 1st-Order EMA Low-Pass Filter
    float raw_derivative = (current_error - previous_error) / dt;
    float filtered_derivative = (ALPHA * raw_derivative) + ((1.0f - ALPHA) * prev_d_filtered);

    prev_d_filtered = filtered_derivative;
    previous_error  = current_error;

    // 5. PD Steering Effort Calculation
    float control_effort = (KP * current_error) + (KD * filtered_derivative);

    // 6. Differential Speed Mixing & Clamping
    int left_command  = BASE_SPEED + (int)control_effort;
    int right_command = BASE_SPEED - (int)control_effort;

    left_command  = constrain(left_command, MIN_PWM, MAX_PWM);
    right_command = constrain(right_command, MIN_PWM, MAX_PWM);

    // 7. Dispatch to Motor Driver
    set_motor_output(left_command, right_command);
}

// Hardware Abstraction Layer (HAL): TB6612FNG Driver
void set_motor_output(int left_pwm, int right_pwm) {
    // Left Channel (Motor A)
    if (left_pwm >= 0) {
        digitalWrite(PIN_AIN1, HIGH);
        digitalWrite(PIN_AIN2, LOW);
    } else {
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, HIGH);
    }
    analogWrite(PIN_PWMA, abs(left_pwm));

    // Right Channel (Motor B)
    if (right_pwm >= 0) {
        digitalWrite(PIN_BIN1, HIGH);
        digitalWrite(PIN_BIN2, LOW);
    } else {
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, HIGH);
    }
    analogWrite(PIN_PWMB, abs(right_pwm));
}

void apply_active_brake() {
    // TB6612FNG Low-Side MOSFET Short-Brake Mode
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    analogWrite(PIN_PWMA, 0);

    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    analogWrite(PIN_PWMB, 0);
}