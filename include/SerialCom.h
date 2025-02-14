#ifndef SERIALCOM_H
#define SERIALCOM_H

#include <Arduino.h>

class SerialCom {
public:
    static constexpr uint32_t BAUD_RATE = 115200;
    static constexpr size_t RX_BUFFER_SIZE = 256;

    SerialCom();
    ~SerialCom();

    // Initialize serial communication
    bool init();

    // Send data
    void send(const char* data);
    void sendln(const char* data);  // Send with newline
    void printf(const char* format, ...);  // Formatted output

    // Read operations
    bool dataAvailable() const;
    size_t read(char* buffer, size_t max_length);
    char read_char();

    // Process received data
    void process();

    // Utility functions
    void flush();

private:
    bool initialized;
    char rxBuffer[RX_BUFFER_SIZE];
    size_t rxIndex;

    // Prevent copying
    SerialCom(const SerialCom&) = delete;
    SerialCom& operator=(const SerialCom&) = delete;
};

#endif // SERIALCOM_H
