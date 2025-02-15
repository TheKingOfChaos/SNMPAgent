#ifndef CIRCUIT_PROTECTION_H
#define CIRCUIT_PROTECTION_H

#include <Arduino.h>
#include <cstdint>

class CircuitProtection {
public:
    // Protection types
    enum class ProtectionType {
        INPUT_WITH_PULLUP,
        INPUT_WITH_PULLDOWN,
        OUTPUT_WITH_CURRENT_LIMIT,
        ISOLATED_INPUT
    };
    
    // Interrupt modes
    enum class InterruptMode {
        NONE,
        RISING,
        FALLING,
        BOTH_EDGES,  // Same as CHANGE
        CHANGE = BOTH_EDGES
    };
    
    // Protection configuration
    struct ProtectionConfig {
        ProtectionType type;
        InterruptMode interruptMode = InterruptMode::NONE;
        float maxVoltage = 3.3f;          // Maximum allowed voltage
        uint8_t currentLimit = 0;         // Current limit in mA (0 = no limit)
        uint32_t maxTriggers = 5;         // Maximum triggers before fault
        uint32_t triggerWindow = 1000;    // Time window for trigger counting (ms)
    };
    
    // Pin status information
    struct PinStatus {
        bool enabled;
        ProtectionType type;
        uint32_t triggerCount;
        unsigned long lastTrigger;
    };
    
    // Callback type for fault notification
    using FaultCallback = void (*)(uint8_t pin);
    
    CircuitProtection();
    
    // Pin protection methods
    bool protectPin(uint8_t pin, ProtectionConfig config);
    void unprotectPin(uint8_t pin);
    bool checkVoltage(uint8_t pin) const;
    bool hasErrors(uint8_t pin) const;
    PinStatus getPinStatus(uint8_t pin) const;
    void resetTriggerCount(uint8_t pin);
    
    // Fault callback registration
    void setFaultCallback(FaultCallback callback) {
        faultCallback_ = callback;
    }
    
private:
    static constexpr size_t MAX_PROTECTED_PINS = 16;
    static constexpr unsigned long DEBOUNCE_TIME = 50;  // ms
    
    // Protected pin information
    struct ProtectedPin {
        uint8_t pin;
        ProtectionConfig config;
        unsigned long lastTrigger;
        uint32_t triggerCount;
        bool enabled;
    };
    
    ProtectedPin protectedPins_[MAX_PROTECTED_PINS];
    FaultCallback faultCallback_;
    
    void handleInterrupt(size_t index);
};

#endif // CIRCUIT_PROTECTION_H
