#include "Arduino.h"
#include <chrono>
#include <thread>

// Global Serial instance
HardwareSerial Serial;

// Arduino function implementations
void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned long millis(void) {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}
