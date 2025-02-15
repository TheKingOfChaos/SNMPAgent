#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <stdio.h>

// Basic Arduino types
typedef bool boolean;
typedef uint8_t byte;

// Arduino functions
void delay(unsigned long ms);
unsigned long millis(void);

// Serial class mock
class HardwareSerial {
public:
    void begin(unsigned long baud) {}
    void write(uint8_t c) {}
    void flush() {}
};

extern HardwareSerial Serial;

#endif // ARDUINO_H
