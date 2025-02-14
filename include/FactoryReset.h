#pragma once

#include "Settings.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

class FactoryResetHandler {
public:
    // Initialize with GPIO pins and settings manager reference
    FactoryResetHandler(SettingsManager& settings);
    
    // Call this periodically to check reset button state
    void checkResetButton();

private:
    static constexpr uint8_t RESET_PIN = 22;        // GPIO22 for factory reset button
    static constexpr uint8_t LED_PIN = 25;          // Onboard LED
    static constexpr uint32_t RESET_HOLD_TIME = 10000; // 10 seconds in milliseconds
    
    SettingsManager& settingsManager;
    uint32_t resetButtonPressStart;
    bool resetInProgress;
    
    void initializePins();
    void handleResetComplete();
    void updateLedFeedback(uint32_t holdDuration);
};
