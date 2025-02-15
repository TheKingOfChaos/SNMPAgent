#include "PowerMonitor.h"
#include "InterruptHandler.h"
#include <Arduino.h>

PowerMonitor::PowerMonitor(MIB& mib) : mib_(mib), lastInterruptTime_(0) {
    initializeMIBNodes();
}

void PowerMonitor::initializeMIBNodes() {
    // Initialize MIB nodes
    mib_.registerNode(POWER_STATE_OID, MIB::NodeType::INTEGER, MIB::Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(POWER_STATE_ON);
            return value;
        });
    
    mib_.registerNode(LAST_POWER_LOSS_OID, MIB::NodeType::INTEGER, MIB::Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(0);
            return value;
        });
    
    mib_.registerNode(POWER_LOSS_COUNT_OID, MIB::NodeType::INTEGER, MIB::Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(0);
            return value;
        });
}

void PowerMonitor::begin() {
    // Configure GPIO27 for power monitoring with internal pull-up
    pinMode(POWER_PIN, INPUT_PULLUP);
    
    // Create static method for interrupt handling
    static PowerMonitor* instance = this;
    static auto handler = []() {
        instance->handleInterrupt();
    };
    
    // Attach interrupt handler
    InterruptHandler::getInstance().attachInterrupt(
        POWER_PIN,
        handler,
        InterruptHandler::Mode::CHANGE
    );
}

void PowerMonitor::handleInterrupt() {
    unsigned long now = millis();
    
    // Debounce: ignore interrupts within 50ms of the last one
    if (now - lastInterruptTime_ < DEBOUNCE_TIME) {
        return;
    }
    lastInterruptTime_ = now;
    
    // Read current power state
    bool powerPresent = (digitalRead(POWER_PIN) == HIGH);
    
    // Update power state in MIB
    ASN1Object stateValue(ASN1Object::Type::INTEGER);
    stateValue.setInteger(powerPresent ? POWER_STATE_ON : POWER_STATE_OFF);
    mib_.setValue(POWER_STATE_OID, stateValue);
    
    if (!powerPresent) {
        // Update last power loss time
        ASN1Object timeValue(ASN1Object::Type::INTEGER);
        timeValue.setInteger(now);
        mib_.setValue(LAST_POWER_LOSS_OID, timeValue);
        
        // Increment power loss count
        ASN1Object countValue;
        if (mib_.getValue(POWER_LOSS_COUNT_OID, countValue)) {
            int32_t currentCount = countValue.getInteger();
            countValue.setInteger(currentCount + 1);
            mib_.setValue(POWER_LOSS_COUNT_OID, countValue);
        }
    }
}

bool PowerMonitor::isPowerPresent() const {
    return digitalRead(POWER_PIN) == HIGH;
}

uint32_t PowerMonitor::getPowerLossCount() const {
    ASN1Object value;
    if (mib_.getValue(POWER_LOSS_COUNT_OID, value)) {
        return value.getInteger();
    }
    return 0;
}

uint32_t PowerMonitor::getLastPowerLossTime() const {
    ASN1Object value;
    if (mib_.getValue(LAST_POWER_LOSS_OID, value)) {
        return value.getInteger();
    }
    return 0;
}
