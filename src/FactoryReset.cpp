#include "FactoryReset.h"

FactoryResetHandler::FactoryResetHandler(SettingsManager& settings)
    : settingsManager(settings)
    , resetButtonPressStart(0)
    , resetInProgress(false) {
    initializePins();
}

void FactoryResetHandler::initializePins() {
    // Initialize reset button pin as input with pull-up
    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_IN);
    gpio_pull_up(RESET_PIN);
    
    // Initialize LED pin as output
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0); // LED off by default
}

void FactoryResetHandler::checkResetButton() {
    // Button is active low since we're using pull-up
    bool buttonPressed = !gpio_get(RESET_PIN);
    uint32_t currentTime = to_ms_since_boot(get_absolute_time());
    
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
        gpio_put(LED_PIN, 0); // Turn off LED
    }
}

void FactoryResetHandler::updateLedFeedback(uint32_t holdDuration) {
    // LED blink frequency increases as hold time progresses
    // Start with 1Hz and increase to 4Hz over the hold duration
    uint32_t blinkPeriod = RESET_HOLD_TIME / (holdDuration / 1000 + 1);
    bool ledState = (holdDuration % blinkPeriod) < (blinkPeriod / 2);
    gpio_put(LED_PIN, ledState);
}

void FactoryResetHandler::handleResetComplete() {
    // Rapid LED flash to indicate reset in progress
    for (int i = 0; i < 5; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    // Perform factory reset
    settingsManager.factoryReset();
    
    // Long LED flash to indicate reset complete
    gpio_put(LED_PIN, 1);
    sleep_ms(1000);
    gpio_put(LED_PIN, 0);
    
    // Reset state
    resetInProgress = false;
}
