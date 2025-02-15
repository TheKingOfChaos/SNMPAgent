#include <unity.h>
#include <Arduino.h>
#include "CircuitProtection.h"
#include "../mock/mock_gpio.h"
#include "../mock/test_utils.h"

// Global instances
static CircuitProtection* protection;
static MockGPIO gpio;
static bool faultDetected;
static uint8_t faultPin;

void faultCallback(uint8_t pin) {
    faultDetected = true;
    faultPin = pin;
}

void setUp(void) {
    protection = new CircuitProtection();
    protection->setFaultCallback(faultCallback);
    faultDetected = false;
    faultPin = 0;
    TestUtils::startMemoryTracking();
}

void tearDown(void) {
    delete protection;
}

void test_input_protection() {
    // Configure GPIO27 with pullup protection
    CircuitProtection::ProtectionConfig config = {
        .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP,
        .interruptMode = CircuitProtection::InterruptMode::FALLING,
        .maxVoltage = 3.3f
    };
    
    TEST_ASSERT_TRUE(protection->protectPin(27, config));
    
    // Verify pin configuration
    auto status = protection->getPinStatus(27);
    TEST_ASSERT_TRUE(status.enabled);
    TEST_ASSERT_EQUAL(CircuitProtection::ProtectionType::INPUT_WITH_PULLUP, status.type);
    
    // Simulate voltage within limits
    TEST_ASSERT_TRUE(protection->checkVoltage(27));
    
    // Verify no fault condition
    TEST_ASSERT_FALSE(faultDetected);
}

void test_output_current_limiting() {
    // Configure GPIO16 with current limiting
    CircuitProtection::ProtectionConfig config = {
        .type = CircuitProtection::ProtectionType::OUTPUT_WITH_CURRENT_LIMIT,
        .currentLimit = 20  // 20mA limit
    };
    
    TEST_ASSERT_TRUE(protection->protectPin(16, config));
    
    // Verify pin configuration
    auto status = protection->getPinStatus(16);
    TEST_ASSERT_TRUE(status.enabled);
    TEST_ASSERT_EQUAL(CircuitProtection::ProtectionType::OUTPUT_WITH_CURRENT_LIMIT, status.type);
    
    // Verify current limiting PWM setup
    // Note: In real hardware, would measure actual current
    TEST_ASSERT_TRUE(protection->checkVoltage(16));
}

void test_fault_detection() {
    // Configure GPIO27 with fault detection
    CircuitProtection::ProtectionConfig config = {
        .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP,
        .interruptMode = CircuitProtection::InterruptMode::CHANGE,
        .maxTriggers = 3,
        .triggerWindow = 1000
    };
    
    TEST_ASSERT_TRUE(protection->protectPin(27, config));
    
    // Simulate multiple transitions within window
    for (int i = 0; i < 4; i++) {
        gpio.simulateInterrupt(27, i % 2 ? HIGH : LOW);
        delay(100);
    }
    
    // Verify fault detection
    TEST_ASSERT_TRUE(faultDetected);
    TEST_ASSERT_EQUAL(27, faultPin);
    
    // Verify pin was disabled
    auto status = protection->getPinStatus(27);
    TEST_ASSERT_FALSE(status.enabled);
}

void test_voltage_monitoring() {
    // Configure GPIO27 with voltage monitoring
    CircuitProtection::ProtectionConfig config = {
        .type = CircuitProtection::ProtectionType::ISOLATED_INPUT,
        .maxVoltage = 3.3f
    };
    
    TEST_ASSERT_TRUE(protection->protectPin(27, config));
    
    // Simulate voltage readings
    gpio.analogWrite(27, 512);  // ~1.65V
    TEST_ASSERT_TRUE(protection->checkVoltage(27));
    
    gpio.analogWrite(27, 1023); // ~3.3V
    TEST_ASSERT_TRUE(protection->checkVoltage(27));
    
    gpio.analogWrite(27, 1100); // >3.3V
    TEST_ASSERT_FALSE(protection->checkVoltage(27));
}

void test_multiple_pins() {
    // Configure multiple pins with different protections
    CircuitProtection::ProtectionConfig config1 = {
        .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP
    };
    
    CircuitProtection::ProtectionConfig config2 = {
        .type = CircuitProtection::ProtectionType::OUTPUT_WITH_CURRENT_LIMIT,
        .currentLimit = 10
    };
    
    TEST_ASSERT_TRUE(protection->protectPin(27, config1));
    TEST_ASSERT_TRUE(protection->protectPin(16, config2));
    
    // Verify both pins are protected
    auto status1 = protection->getPinStatus(27);
    auto status2 = protection->getPinStatus(16);
    
    TEST_ASSERT_TRUE(status1.enabled);
    TEST_ASSERT_TRUE(status2.enabled);
    
    // Verify correct protection types
    TEST_ASSERT_EQUAL(CircuitProtection::ProtectionType::INPUT_WITH_PULLUP, status1.type);
    TEST_ASSERT_EQUAL(CircuitProtection::ProtectionType::OUTPUT_WITH_CURRENT_LIMIT, status2.type);
}

void test_trigger_counting() {
    // Configure pin with trigger counting
    CircuitProtection::ProtectionConfig config = {
        .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP,
        .interruptMode = CircuitProtection::InterruptMode::CHANGE,
        .maxTriggers = 5,
        .triggerWindow = 2000
    };
    
    TEST_ASSERT_TRUE(protection->protectPin(27, config));
    
    // Generate some triggers
    for (int i = 0; i < 3; i++) {
        gpio.simulateInterrupt(27, i % 2 ? HIGH : LOW);
        delay(100);
    }
    
    // Verify trigger count
    auto status = protection->getPinStatus(27);
    TEST_ASSERT_EQUAL(3, status.triggerCount);
    
    // Reset trigger count
    protection->resetTriggerCount(27);
    status = protection->getPinStatus(27);
    TEST_ASSERT_EQUAL(0, status.triggerCount);
}

void test_memory_usage() {
    // Add maximum number of protected pins
    for (int i = 0; i < 16; i++) {
        CircuitProtection::ProtectionConfig config = {
            .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP
        };
        TEST_ASSERT_TRUE(protection->protectPin(i, config));
    }
    
    // Verify memory usage is within limits
    size_t memUsed = TestUtils::getMemoryUsage();
    TEST_ASSERT_LESS_THAN(135168, memUsed);  // Under 50% RAM limit
    
    // Try to add one more pin (should fail)
    CircuitProtection::ProtectionConfig config = {
        .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP
    };
    TEST_ASSERT_FALSE(protection->protectPin(16, config));
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_input_protection);
    RUN_TEST(test_output_current_limiting);
    RUN_TEST(test_fault_detection);
    RUN_TEST(test_voltage_monitoring);
    RUN_TEST(test_multiple_pins);
    RUN_TEST(test_trigger_counting);
    RUN_TEST(test_memory_usage);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
