#include <unity.h>
#include <Arduino.h>
#include "../mock/mock_gpio.h"
#include "MIB.h"
#include "ASN1Types.h"
#include "../mock/test_utils.h"
#include "../unit/test_mib/test_wrapper.h"

// Global instances
static MockGPIO gpio;
static MIB mib;

// Power monitoring OIDs
const std::vector<uint32_t> powerStateOID = {1, 3, 6, 1, 4, 1, 63050, 1, 1, 0}; // powerState.0
const std::vector<uint32_t> lastPowerLossOID = {1, 3, 6, 1, 4, 1, 63050, 1, 2, 0}; // lastPowerLoss.0
const std::vector<uint32_t> powerLossCountOID = {1, 3, 6, 1, 4, 1, 63050, 1, 3, 0}; // powerLossCount.0

// Power state values
const uint8_t POWER_STATE_ON = 1;
const uint8_t POWER_STATE_OFF = 0;

// Callback function for power state changes
static void updatePowerState(bool powerPresent) {
    ASN1::Integer* stateValue = new ASN1::Integer(powerPresent ? POWER_STATE_ON : POWER_STATE_OFF);
    ASN1TypeWrapper* stateWrapper = new ASN1TypeWrapper(stateValue);
    mib.setValue(powerStateOID, stateWrapper);
    
    if (!powerPresent) {
        // Update last power loss time
        ASN1::Integer* timeValue = new ASN1::Integer(millis());
        ASN1TypeWrapper* timeWrapper = new ASN1TypeWrapper(timeValue);
        mib.setValue(lastPowerLossOID, timeWrapper);
        
        // Increment power loss count
        ASN1Object* countObj = mib.getValue(powerLossCountOID);
        if (countObj) {
            ASN1TypeWrapper* countWrapper = dynamic_cast<ASN1TypeWrapper*>(countObj);
            if (countWrapper) {
                const ASN1::Integer* countValue = dynamic_cast<const ASN1::Integer*>(countWrapper->getValue());
                if (countValue) {
                    ASN1::Integer* newCount = new ASN1::Integer(countValue->getValue() + 1);
                    ASN1TypeWrapper* newCountWrapper = new ASN1TypeWrapper(newCount);
                    mib.setValue(powerLossCountOID, newCountWrapper);
                }
            }
        }
    }
}

void setUp(void) {
    gpio = MockGPIO();
    mib.initialize();
    
    // Initialize power monitoring MIB nodes
    mib.addNode(powerStateOID, "powerState", NodeType::SCALAR, AccessType::READ_ONLY);
    mib.addNode(lastPowerLossOID, "lastPowerLoss", NodeType::SCALAR, AccessType::READ_ONLY);
    mib.addNode(powerLossCountOID, "powerLossCount", NodeType::SCALAR, AccessType::READ_ONLY);
    
    // Initialize power loss count to 0
    ASN1::Integer* countValue = new ASN1::Integer(0);
    ASN1TypeWrapper* countWrapper = new ASN1TypeWrapper(countValue);
    mib.setValue(powerLossCountOID, countWrapper);
    
    TestUtils::startMemoryTracking();
}

void tearDown(void) {
    // Nothing to clean up
}

void test_power_pin_configuration() {
    // Configure GPIO27 for power monitoring
    gpio.pinMode(27, MockGPIO::INPUT_PULLUP_PIN);
    
    // Verify pin configuration
    TEST_ASSERT_EQUAL(MockGPIO::INPUT_PULLUP_PIN, gpio.getPinMode(27));
    TEST_ASSERT_EQUAL(HIGH, gpio.digitalRead(27));
}

void test_interrupt_handling() {
    // Configure GPIO27
    gpio.pinMode(27, MockGPIO::INPUT_PULLUP_PIN);
    
    // Set up interrupt handler
    gpio.attachInterrupt(27, []() {
        updatePowerState(gpio.digitalRead(27) == HIGH);
    }, MockGPIO::BOTH_EDGES);
    
    // Verify interrupt configuration
    TEST_ASSERT_EQUAL(MockGPIO::BOTH_EDGES, gpio.getInterruptMode(27));
    TEST_ASSERT_TRUE(gpio.hasCallback(27));
    
    // Initial state
    ASN1Object* stateObj = mib.getValue(powerStateOID);
    TEST_ASSERT_NOT_NULL(stateObj);
    ASN1TypeWrapper* stateWrapper = dynamic_cast<ASN1TypeWrapper*>(stateObj);
    TEST_ASSERT_NOT_NULL(stateWrapper);
    const ASN1::Integer* stateValue = dynamic_cast<const ASN1::Integer*>(stateWrapper->getValue());
    TEST_ASSERT_NOT_NULL(stateValue);
    TEST_ASSERT_EQUAL(POWER_STATE_ON, stateValue->getValue());
}

void test_power_loss_detection() {
    // Configure GPIO27
    gpio.pinMode(27, MockGPIO::INPUT_PULLUP_PIN);
    gpio.attachInterrupt(27, []() {
        updatePowerState(gpio.digitalRead(27) == HIGH);
    }, MockGPIO::BOTH_EDGES);
    
    // Initial state
    TEST_ASSERT_EQUAL(HIGH, gpio.digitalRead(27));
    
    // Simulate power loss
    unsigned long lossTime = millis();
    gpio.simulateInterrupt(27, LOW);
    
    // Verify power state update
    ASN1Object* stateObj = mib.getValue(powerStateOID);
    TEST_ASSERT_NOT_NULL(stateObj);
    ASN1TypeWrapper* stateWrapper = dynamic_cast<ASN1TypeWrapper*>(stateObj);
    TEST_ASSERT_NOT_NULL(stateWrapper);
    const ASN1::Integer* stateValue = dynamic_cast<const ASN1::Integer*>(stateWrapper->getValue());
    TEST_ASSERT_NOT_NULL(stateValue);
    TEST_ASSERT_EQUAL(POWER_STATE_OFF, stateValue->getValue());
    
    // Verify last power loss time
    ASN1Object* timeObj = mib.getValue(lastPowerLossOID);
    TEST_ASSERT_NOT_NULL(timeObj);
    ASN1TypeWrapper* timeWrapper = dynamic_cast<ASN1TypeWrapper*>(timeObj);
    TEST_ASSERT_NOT_NULL(timeWrapper);
    const ASN1::Integer* timeValue = dynamic_cast<const ASN1::Integer*>(timeWrapper->getValue());
    TEST_ASSERT_NOT_NULL(timeValue);
    TEST_ASSERT_UINT32_WITHIN(100, lossTime, timeValue->getValue());
    
    // Verify power loss count increment
    ASN1Object* countObj = mib.getValue(powerLossCountOID);
    TEST_ASSERT_NOT_NULL(countObj);
    ASN1TypeWrapper* countWrapper = dynamic_cast<ASN1TypeWrapper*>(countObj);
    TEST_ASSERT_NOT_NULL(countWrapper);
    const ASN1::Integer* countValue = dynamic_cast<const ASN1::Integer*>(countWrapper->getValue());
    TEST_ASSERT_NOT_NULL(countValue);
    TEST_ASSERT_EQUAL(1, countValue->getValue());
}

void test_debounce_behavior() {
    // Configure GPIO27
    gpio.pinMode(27, MockGPIO::INPUT_PULLUP_PIN);
    gpio.attachInterrupt(27, []() {
        updatePowerState(gpio.digitalRead(27) == HIGH);
    }, MockGPIO::BOTH_EDGES);
    
    // Initial state
    TEST_ASSERT_EQUAL(HIGH, gpio.digitalRead(27));
    
    // Simulate rapid power fluctuations
    for (int i = 0; i < 10; i++) {
        gpio.simulateInterrupt(27, LOW);
        delay(10);  // 10ms between transitions
        gpio.simulateInterrupt(27, HIGH);
        delay(10);
    }
    
    // Verify power loss count (should be limited by debouncing)
    ASN1Object* countObj = mib.getValue(powerLossCountOID);
    TEST_ASSERT_NOT_NULL(countObj);
    ASN1TypeWrapper* countWrapper = dynamic_cast<ASN1TypeWrapper*>(countObj);
    TEST_ASSERT_NOT_NULL(countWrapper);
    const ASN1::Integer* countValue = dynamic_cast<const ASN1::Integer*>(countWrapper->getValue());
    TEST_ASSERT_NOT_NULL(countValue);
    TEST_ASSERT_LESS_THAN(5, countValue->getValue());  // Should be much less than 10 due to debouncing
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_power_pin_configuration);
    RUN_TEST(test_interrupt_handling);
    RUN_TEST(test_power_loss_detection);
    RUN_TEST(test_debounce_behavior);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
