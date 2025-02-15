#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#include <functional>
#include <map>
#include <Arduino.h>

// Mock GPIO interface for testing
class MockGPIO {
public:
    // Pin modes
    enum PinMode {
        INPUT_PIN,
        OUTPUT_PIN,
        INPUT_PULLUP_PIN,
        INPUT_PULLDOWN_PIN
    };
    
    // Interrupt modes
    enum InterruptMode {
        RISING_EDGE,
        FALLING_EDGE,
        BOTH_EDGES,
        NO_INTERRUPT
    };
    
    MockGPIO() {
        reset();
    }
    
    // Reset all pins to default state
    void reset() {
        for (uint8_t pin = 0; pin < 30; pin++) {  // RP2040 has 30 GPIO pins
            pinModes[pin] = INPUT_PIN;
            pinValues[pin] = LOW;
            interruptModes[pin] = NO_INTERRUPT;
            interruptCallbacks[pin] = nullptr;
            lastInterruptTime[pin] = 0;
        }
    }
    
    // Pin configuration
    void pinMode(uint8_t pin, PinMode mode) {
        if (pin < 30) {
            pinModes[pin] = mode;
            // Set default value based on mode
            if (mode == INPUT_PULLUP_PIN) {
                pinValues[pin] = HIGH;
            } else if (mode == INPUT_PULLDOWN_PIN) {
                pinValues[pin] = LOW;
            }
        }
    }
    
    // Digital read/write
    int digitalRead(uint8_t pin) {
        return (pin < 30) ? pinValues[pin] : LOW;
    }
    
    void digitalWrite(uint8_t pin, int value) {
        if (pin < 30 && pinModes[pin] == OUTPUT_PIN) {
            pinValues[pin] = value;
        }
    }
    
    // Interrupt handling
    void attachInterrupt(uint8_t pin, std::function<void()> callback, InterruptMode mode) {
        if (pin < 30) {
            interruptModes[pin] = mode;
            interruptCallbacks[pin] = callback;
        }
    }
    
    void detachInterrupt(uint8_t pin) {
        if (pin < 30) {
            interruptModes[pin] = NO_INTERRUPT;
            interruptCallbacks[pin] = nullptr;
        }
    }
    
    // Test helper methods
    void simulateInterrupt(uint8_t pin, int value) {
        if (pin < 30 && interruptCallbacks[pin] != nullptr) {
            int oldValue = pinValues[pin];
            pinValues[pin] = value;
            
            // Check if interrupt should trigger based on mode
            bool shouldTrigger = false;
            switch (interruptModes[pin]) {
                case RISING_EDGE:
                    shouldTrigger = (oldValue == LOW && value == HIGH);
                    break;
                case FALLING_EDGE:
                    shouldTrigger = (oldValue == HIGH && value == LOW);
                    break;
                case BOTH_EDGES:
                    shouldTrigger = (oldValue != value);
                    break;
                default:
                    break;
            }
            
            // Call interrupt callback if conditions are met
            if (shouldTrigger) {
                unsigned long now = millis();
                // Debounce: ignore interrupts within 50ms of the last one
                if (now - lastInterruptTime[pin] >= 50) {
                    interruptCallbacks[pin]();
                    lastInterruptTime[pin] = now;
                }
            }
        }
    }
    
    PinMode getPinMode(uint8_t pin) {
        return (pin < 30) ? pinModes[pin] : INPUT_PIN;
    }
    
    InterruptMode getInterruptMode(uint8_t pin) {
        return (pin < 30) ? interruptModes[pin] : NO_INTERRUPT;
    }
    
    bool hasCallback(uint8_t pin) {
        return (pin < 30) && (interruptCallbacks[pin] != nullptr);
    }
    
    // Analog functions
    void analogWrite(uint8_t pin, uint16_t value) {
        if (pin < 30) {
            analogValues[pin] = value;
        }
    }
    
    uint16_t analogRead(uint8_t pin) {
        return (pin < 30) ? analogValues[pin] : 0;
    }
    
private:
    std::map<uint8_t, PinMode> pinModes;
    std::map<uint8_t, int> pinValues;
    std::map<uint8_t, uint16_t> analogValues;  // 0-1023 range
    std::map<uint8_t, InterruptMode> interruptModes;
    std::map<uint8_t, std::function<void()>> interruptCallbacks;
    std::map<uint8_t, unsigned long> lastInterruptTime;
};

#endif // MOCK_GPIO_H
