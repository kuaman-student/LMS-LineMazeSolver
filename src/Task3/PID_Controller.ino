#include <Arduino.h>

const int SENSOR_PINS[5] = {34, 35, 32, 33, 25};

// Toshiba TB6612FNG Motor Driver Pins
const int PIN_PWMA = 14;
const int PIN_AIN1 = 27;
const int PIN_AIN2 = 26;
const int PIN_PWMB = 13;
const int PIN_BIN1 = 12;
const int PIN_BIN2 = 11;

const float WEIGHTS[5] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};

// PD Controller Gains
const float KP = 48.0f;
const float KD = 24.0f;

// Exponential Moving Average (EMA) Smoothing Factor for Derivative Filter
const float ALPHA = 0.70f;

// Operational Speed Bounds
const int BASE_SPEED   = 180;
const int MAX_PWM      = 255;
const int MIN_PWM      = -255;
const int SEARCH_SPEED = 120;

// Track Loss Detection Thresholds
const int   LOST_LINE_THRESHOLD_SUM = 150;   // Threshold on normalized sum
const unsigned long LOST_TIMEOUT_MS  = 1200;  // 1.2s watchdog safety stop


// State Variables
float previous_error    = 0.0f;
float prev_d_filtered   = 0.0f;
unsigned long last_loop_time_us = 0;
unsigned long line_lost_start_ms = 0;
bool  is_line_lost      = false;


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

    // Initial state: Motors held in active short-brake
    apply_active_brake();

    last_loop_time_us = micros();
}

void loop() {
    // 1. Deterministic Control Loop Timing (200 Hz -> dt = 5 ms)
    unsigned long current_time_us = micros();
    float dt = (current_time_us - last_loop_time_us) / 1000000.0f;
    if (dt < 0.005f) {
        return;
    }
    last_loop_time_us = current_time_us;

    // 2. Read Sensors and Compute Normalized Weighted Centroid
    float weighted_sum = 0.0f;
    float total_sum    = 0.0f;

    for (int i = 0; i < 5; i++) {
        // Normalize 12-bit ADC (0-4095) to calibrated reflectance scale (0-1000)
        int calibrated_reading = map(analogRead(SENSOR_PINS[i]), 0, 4095, 0, 1000);
        weighted_sum += (float)calibrated_reading * WEIGHTS[i];
        total_sum    += (float)calibrated_reading;
    }

    // 3. Lost-Line Condition Handling & Watchdog FSM
    if (total_sum < LOST_LINE_THRESHOLD_SUM) {
        if (!is_line_lost) {
            is_line_lost = true;
            line_lost_start_ms = millis();
        }

        // Safety Watchdog: Shut down motors if line is not recovered within 1.2s
        if (millis() - line_lost_start_ms > LOST_TIMEOUT_MS) {
            apply_active_brake();
            return;
        }

        
        if (previous_error >= 0.0f) {
            // Line slipped to the right -> Pivot clockwise in place
            set_motor_output(SEARCH_SPEED, -SEARCH_SPEED);
        } else {
            // Line slipped to the left -> Pivot counter-clockwise in place
            set_motor_output(-SEARCH_SPEED, SEARCH_SPEED);
        }
        return;
    }

    // Line is detected: clear lost-line flags
    is_line_lost = false;

    // 4. Centroid Error Formulation (Setpoint = 0.0)
    float current_error = weighted_sum / total_sum; // Bounded [-2.0, +2.0]

    // 5. Derivative Calculation with 1st-Order Low-Pass Filter (EMA)
    float raw_derivative = (current_error - previous_error) / dt;
    float filtered_derivative = (ALPHA * raw_derivative) + ((1.0f - ALPHA) * prev_d_filtered);

    prev_d_filtered = filtered_derivative;
    previous_error  = current_error;

    // 6. Compute PD Control Effort
    float control_effort = (KP * current_error) + (KD * filtered_derivative);

    // 7. Differential Motor Speed Mixing
    int left_command  = BASE_SPEED + (int)control_effort;
    int right_command = BASE_SPEED - (int)control_effort;

    // 8. Bi-Directional Actuator Clamping
    left_command  = constrain(left_command, MIN_PWM, MAX_PWM);
    right_command = constrain(right_command, MIN_PWM, MAX_PWM);

    // 9. Dispatch to Motor Driver
    set_motor_output(left_command, right_command);
}


// Hardware Abstraction Layer (HAL): Motor Driver Control
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
    // TB6612FNG Short Brake Mode: IN1 = HIGH, IN2 = HIGH, PWM = 0
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    analogWrite(PIN_PWMA, 0);

    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    analogWrite(PIN_PWMB, 0);
}