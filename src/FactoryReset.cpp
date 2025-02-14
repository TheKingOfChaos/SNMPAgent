#include "FactoryReset.h"

FactoryResetHandler::FactoryResetHandler(SettingsManager& settings)
    : settingsManager(settings)
    , resetButtonPressStart(0)
    , resetInProgress(false) {
    initializePins();
}

void FactoryResetHandler::initializePins() {
    // Initialize reset button pin as input with pull-up
    pinMode(RESET_PIN, INPUT_PULLUP);
    
    // Initialize LED pin as output
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // LED off by default
}

void FactoryResetHandler::checkResetButton() {
    // Button is active low since we're using pull-up
    bool buttonPressed = !digitalRead(RESET_PIN);
    uint32_t currentTime = millis();
    
    if (buttonPressed) {
        if (!resetInProgress) {
            // Button just pressed, start timing
            resetInProgress = true;
            resetButtonPressStart = currentTime;
        } else {
            // Button still held, update LED feedback
            uint32_t holdDuration = currentTime - resetButtonPressStart;
            updateLedFeedback(holdDuration);
            
            // Check if held long enough
            if (holdDuration >= RESET_HOLD_TIME) {
                handleResetComplete();
            }
        }
    } else if (resetInProgress) {
        // Button released before reset time reached
        resetInProgress = false;
        digitalWrite(LED_PIN, LOW); // Turn off LED
    }
}

void FactoryResetHandler::updateLedFeedback(uint32_t holdDuration) {
    // LED blink frequency increases as hold time progresses
    // Start with 1Hz and increase to 4Hz over the hold duration
    uint32_t blinkPeriod = RESET_HOLD_TIME / (holdDuration / 1000 + 1);
    bool ledState = (holdDuration % blinkPeriod) < (blinkPeriod / 2);
    digitalWrite(LED_PIN, ledState);
}

void FactoryResetHandler::handleResetComplete() {
    // Rapid LED flash to indicate reset in progress
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    // Perform factory reset
    settingsManager.factoryReset();
    
    // Long LED flash to indicate reset complete
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    
    // Reset state
    resetInProgress = false;
}
