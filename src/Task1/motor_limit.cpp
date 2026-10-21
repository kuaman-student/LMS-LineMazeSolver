#include <bits/stdc++.h>
using namespace std;

// Limit a signed motor command to the range [-maxMag, +maxMag].
// The sign is kept, only the size is reduced if it is too big.
int clampCommand(int command, int maxMag) {
    if (command > maxMag) {
        return maxMag;
    }
    if (command < -maxMag) {
        return -maxMag;
    }
    return command;
}
//or in short form this function can be written as : max(-M, min(u, M))

int main() {
    int left, right, maxMag;

    cout << "Enter left command, right command, max magnitude: ";
    if (!(cin >> left >> right >> maxMag)) {
        cout << "Please enter three valid integers.\n";
        return 1;
    }

    // A negative limit makes no sense.
    if (maxMag < 0) {
        cout << "Max magnitude must be 0 or more.\n";
        return 0;
    }

    cout << "Left motor:  " << clampCommand(left, maxMag) << '\n';
    cout << "Right motor: " << clampCommand(right, maxMag) << '\n';

    return 0;
}