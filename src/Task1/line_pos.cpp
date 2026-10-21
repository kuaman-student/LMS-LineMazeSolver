#include <bits/stdc++.h>
using namespace std;

// ADC readings should be between 0 and 4095.
const int MAX_VALUE = 4095;

// These values are used to show errors.
const double SUM_ZERO = -3;
const double INVALID_INPUT = -4;
const double ALL_ZERO = -5;

// Calculate the estimated line position.
double weighted_center(int s1, int s2, int s3, int s4, int s5) {
    int sensor[5] = {s1, s2, s3, s4, s5};
    int weight[5] = {-2, -1, 0, 1, 2};

    int total = 0;
    double weighted_sum = 0;

    // Check readings and calculate the total.
    for (int i = 0; i < 5; i++) {
        if (sensor[i] < 0 || sensor[i] > MAX_VALUE) {
            return INVALID_INPUT;
        }

        total += sensor[i];
        weighted_sum += sensor[i] * weight[i];
    }

    // Avoid dividing by zero.
    if (total == 0 && weighted_sum == 0) {
        return ALL_ZERO;
    }
    if (total == 0) {
        return SUM_ZERO;
    }
    return weighted_sum / total;
}

int main() {
    int S1, S2, S3, S4, S5;

    // Take readings from all five sensors.
    cout << "Enter sensor values (S1 S2 S3 S4 S5): ";
    if (!(cin >> S1 >> S2 >> S3 >> S4 >> S5)) {
        cout << "Please enter five valid numbers.\n";
        return 1;
    }

    double position = weighted_center(S1, S2, S3, S4, S5);

    // if sum is zero, we cannot calculate a position.
    if (position == SUM_ZERO) {
        cout << "All readings sum to zero. Cannot calculate position.\n";
        return 0;
    }

    // Show an error if any reading is out of range.
    if (position == INVALID_INPUT) {
        cout << "Sensor values must be between 0 and 4095.\n";
        return 0;
    }

    // All-zero readings cannot give a position.
    if (position == ALL_ZERO) {
        cout << "All readings are zero. Cannot calculate position.\n";
        return 0;
    }
    


    // These patterns can mean the result is not clear.
    const int STRONG = 3000;

    if (S1 >= STRONG && S2 >= STRONG && S3 >= STRONG &&
        S4 >= STRONG && S5 >= STRONG) {
        cout << "Warning: possible wide line or intersection.\n";
    } else if (S1 >= STRONG && S5 >= STRONG) {
        cout << "Warning: both corner sensors are strong.\n";
        cout << "The position may be confusing.\n";
    }

    // Print the result with 4 decimal places.
    cout << fixed << setprecision(4);
    cout << "Estimated center: " << position << '\n';

    // Tell which side has the stronger overall signal.
    if (position < 0) {
        cout << "Line signal is more towards the left of S3.\n";
    } else if (position > 0) {
        cout << "Line signal is more towards the right of S3.\n";
    } else {
        cout << "Weighted position is at the center (S3).\n";
    }

    return 0;
}
