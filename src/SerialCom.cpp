#include "SerialCom.h"
#include <cstdarg>

SerialCom::SerialCom() : initialized(false), rxIndex(0) {}

SerialCom::~SerialCom() {
    if (initialized) {
        Serial.end();
    }
}

bool SerialCom::init() {
    Serial.begin(BAUD_RATE);
    initialized = true;
    return true;
}

void SerialCom::send(const char* data) {
    if (!initialized || !data) return;
    Serial.print(data);
}

void SerialCom::sendln(const char* data) {
    if (!initialized || !data) return;
    Serial.println(data);
}

void SerialCom::printf(const char* format, ...) {
    if (!initialized || !format) return;
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print(buffer);
}

bool SerialCom::dataAvailable() const {
    return initialized && Serial.available() > 0;
}

size_t SerialCom::read(char* buffer, size_t max_length) {
    if (!initialized || !buffer || max_length == 0) return 0;
    
    size_t count = 0;
    while (count < max_length && Serial.available()) {
        buffer[count++] = Serial.read();
    }
    
    return count;
}

char SerialCom::read_char() {
    if (!initialized || !Serial.available()) return '\0';
    return Serial.read();
}

void SerialCom::process() {
    if (!initialized) return;

    while (Serial.available()) {
        char c = Serial.read();
        // Store in buffer if there's space
        if (rxIndex < RX_BUFFER_SIZE) {
            rxBuffer[rxIndex++] = c;
        }
        // Echo character back for CLI feedback
        Serial.write(c);
    }
}

void SerialCom::flush() {
    if (!initialized) return;
    Serial.flush();
    rxIndex = 0;
}
