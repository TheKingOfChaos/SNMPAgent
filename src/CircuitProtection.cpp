#include "CircuitProtection.h"
#include "InterruptHandler.h"
#include <Arduino.h>

CircuitProtection::CircuitProtection() : faultCallback_(nullptr) {
    // Initialize protection status
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        protectedPins_[i] = {
            .pin = 0,
            .config = {
                .type = ProtectionType::INPUT_WITH_PULLUP,
                .interruptMode = InterruptMode::NONE,
                .maxVoltage = 3.3f,
                .currentLimit = 0,
                .maxTriggers = 5,
                .triggerWindow = 1000
            },
            .lastTrigger = 0,
            .triggerCount = 0,
            .enabled = false
        };
    }
}

bool CircuitProtection::protectPin(uint8_t pin, ProtectionConfig config) {
    // Find available slot or existing pin
    size_t index = MAX_PROTECTED_PINS;
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        if (!protectedPins_[i].enabled) {
            index = i;
            break;
        } else if (protectedPins_[i].pin == pin) {
            return false; // Pin already protected
        }
    }
    
    if (index == MAX_PROTECTED_PINS) {
        return false; // No slots available
    }
    
    // Configure hardware protection
    switch (config.type) {
        case ProtectionType::INPUT_WITH_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;
            
        case ProtectionType::INPUT_WITH_PULLDOWN:
            pinMode(pin, INPUT);
            // Note: RP2040 doesn't have built-in pulldown, external resistor required
            break;
            
        case ProtectionType::OUTPUT_WITH_CURRENT_LIMIT:
            pinMode(pin, OUTPUT);
            // Set PWM frequency to limit current if needed
            if (config.currentLimit > 0) {
                analogWriteFreq(20000); // 20kHz PWM
                analogWrite(pin, map(config.currentLimit, 0, 20, 0, 255)); // Max 20mA
            }
            break;
            
        case ProtectionType::ISOLATED_INPUT:
            pinMode(pin, INPUT);
            break;
    }
    
    // Store protection configuration
    protectedPins_[index] = {
        .pin = pin,
        .config = config,
        .lastTrigger = 0,
        .triggerCount = 0,
        .enabled = true
    };
    
    // Set up interrupt if needed
    if (config.interruptMode != InterruptMode::NONE) {
        InterruptHandler::Mode mode;
        switch (config.interruptMode) {
            case InterruptMode::RISING:
                mode = InterruptHandler::Mode::RISING;
                break;
            case InterruptMode::FALLING:
                mode = InterruptHandler::Mode::FALLING;
                break;
            case InterruptMode::BOTH_EDGES:
                mode = InterruptHandler::Mode::CHANGE;
                break;
            default:
                mode = InterruptHandler::Mode::CHANGE;
        }
        
        // Create static instance and handler for this pin
        static CircuitProtection* instance = this;
        static size_t handlerIndex = index;
        static auto handler = []() {
            instance->handleInterrupt(handlerIndex);
        };
        
        InterruptHandler::getInstance().attachInterrupt(pin, handler, mode);
    }
    
    return true;
}

void CircuitProtection::unprotectPin(uint8_t pin) {
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        if (protectedPins_[i].enabled && protectedPins_[i].pin == pin) {
            // Remove interrupt if present
            if (protectedPins_[i].config.interruptMode != InterruptMode::NONE) {
                InterruptHandler::getInstance().detachInterrupt(pin);
            }
            
            // Reset pin to safe state
            pinMode(pin, INPUT);
            
            // Clear protection
            protectedPins_[i].enabled = false;
            break;
        }
    }
}

void CircuitProtection::handleInterrupt(size_t index) {
    if (index < MAX_PROTECTED_PINS && protectedPins_[index].enabled) {
        ProtectedPin& pin = protectedPins_[index];
        unsigned long now = millis();
        
        // Debounce
        if (now - pin.lastTrigger < DEBOUNCE_TIME) {
            return;
        }
        pin.lastTrigger = now;
        
        // Count triggers
        pin.triggerCount++;
        
        // Check for fault condition
        if (pin.triggerCount >= pin.config.maxTriggers &&
            now - pin.lastTrigger <= pin.config.triggerWindow) {
            // Fault detected - disable pin
            unprotectPin(pin.pin);
            
            // Notify callback if registered
            if (faultCallback_) {
                faultCallback_(pin.pin);
            }
        }
    }
}

bool CircuitProtection::hasErrors(uint8_t pin) const {
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        if (protectedPins_[i].enabled && protectedPins_[i].pin == pin) {
            // Check if trigger count exceeds threshold within window
            if (protectedPins_[i].triggerCount >= protectedPins_[i].config.maxTriggers) {
                unsigned long now = millis();
                if (now - protectedPins_[i].lastTrigger <= protectedPins_[i].config.triggerWindow) {
                    return true;
                }
            }
            
            // Check voltage if applicable
            if (checkVoltage(pin) == false) {
                return true;
            }
            
            return false;  // Pin found and no errors
        }
    }
    return false;  // Pin not found/not protected
}

bool CircuitProtection::checkVoltage(uint8_t pin) const {
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        if (protectedPins_[i].enabled && protectedPins_[i].pin == pin) {
            // Read voltage through voltage divider
            int reading = analogRead(pin);
            float voltage = (reading * 3.3f) / 1024.0f;
            
            // Check against limits
            return voltage <= protectedPins_[i].config.maxVoltage;
        }
    }
    return false;
}

CircuitProtection::PinStatus CircuitProtection::getPinStatus(uint8_t pin) const {
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        if (protectedPins_[i].enabled && protectedPins_[i].pin == pin) {
            return {
                .enabled = true,
                .type = protectedPins_[i].config.type,
                .triggerCount = protectedPins_[i].triggerCount,
                .lastTrigger = protectedPins_[i].lastTrigger
            };
        }
    }
    return {
        .enabled = false,
        .type = ProtectionType::INPUT_WITH_PULLUP,
        .triggerCount = 0,
        .lastTrigger = 0
    };
}

void CircuitProtection::resetTriggerCount(uint8_t pin) {
    for (size_t i = 0; i < MAX_PROTECTED_PINS; i++) {
        if (protectedPins_[i].enabled && protectedPins_[i].pin == pin) {
            protectedPins_[i].triggerCount = 0;
            break;
        }
    }
}
