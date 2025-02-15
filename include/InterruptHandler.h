#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H

#include <Arduino.h>
#include <cstdint>

class InterruptHandler {
public:
    static InterruptHandler& getInstance() {
        static InterruptHandler instance;
        return instance;
    }
    
    // Static interrupt handlers for each pin
    static void handleInterrupt0() { handleInterrupt(0); }
    static void handleInterrupt1() { handleInterrupt(1); }
    static void handleInterrupt2() { handleInterrupt(2); }
    static void handleInterrupt3() { handleInterrupt(3); }
    // ... add more as needed up to the max number of interrupt pins
    
    enum class Mode {
        LOW = 0x0,
        HIGH = 0x1,
        CHANGE = 0x2,
        FALLING = 0x3,
        RISING = 0x4
    };
    
    using InterruptCallback = void (*)(void);
    static constexpr size_t MAX_INTERRUPT_PINS = 32;
    
    void attachInterrupt(uint8_t pin, InterruptCallback callback, Mode mode) {
        if (pin >= MAX_INTERRUPT_PINS) return;
        
        callbacks_[pin] = callback;
        
        // Map pin number to static handler
        void (*handler)() = nullptr;
        switch (pin) {
            case 0: handler = handleInterrupt0; break;
            case 1: handler = handleInterrupt1; break;
            case 2: handler = handleInterrupt2; break;
            case 3: handler = handleInterrupt3; break;
            // ... add more cases as needed
            default: return;
        }
        
        PinStatus arduinoMode;
        switch (mode) {
            case Mode::LOW: arduinoMode = LOW; break;
            case Mode::HIGH: arduinoMode = HIGH; break;
            case Mode::CHANGE: arduinoMode = CHANGE; break;
            case Mode::FALLING: arduinoMode = FALLING; break;
            case Mode::RISING: arduinoMode = RISING; break;
            default: return;
        }
        ::attachInterrupt(digitalPinToInterrupt(pin), handler, arduinoMode);
    }
    
    void detachInterrupt(uint8_t pin) {
        if (pin >= MAX_INTERRUPT_PINS) return;
        ::detachInterrupt(digitalPinToInterrupt(pin));
        callbacks_[pin] = nullptr;
    }
    
private:
    InterruptHandler() {
        for (size_t i = 0; i < MAX_INTERRUPT_PINS; i++) {
            callbacks_[i] = nullptr;
        }
    }
    
    InterruptCallback callbacks_[MAX_INTERRUPT_PINS];
    
    static void handleInterrupt(uint8_t pin) {
        auto& instance = getInstance();
        if (pin < MAX_INTERRUPT_PINS && instance.callbacks_[pin]) {
            instance.callbacks_[pin]();
        }
    }
};

#endif // INTERRUPT_HANDLER_H
