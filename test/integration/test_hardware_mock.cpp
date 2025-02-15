#include <unity.h>
#include <Arduino.h>
#include "../mock/mock_w5500.h"
#include "../mock/mock_gpio.h"
#include "../mock/mock_flash.h"
#include "../mock/test_utils.h"

// Global mock instances
static MockW5500 w5500;
static MockGPIO gpio;
static MockFlash flash;

void setUp(void) {
    w5500 = MockW5500();
    gpio = MockGPIO();
    flash = MockFlash();
    TestUtils::startMemoryTracking();
}

void tearDown(void) {
    // Nothing to clean up
}

void test_network_configuration() {
    // Test DHCP configuration
    uint8_t mac[] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};
    TEST_ASSERT_TRUE(w5500.begin(mac));
    TEST_ASSERT_TRUE(w5500.setDHCP(true));
    
    // Test static IP configuration
    TEST_ASSERT_TRUE(w5500.setDHCP(false));
    TEST_ASSERT_TRUE(w5500.setStaticIP(
        0xC0A80164,  // 192.168.1.100
        0xFFFFFF00,  // 255.255.255.0
        0xC0A80101   // 192.168.1.1
    ));
    
    // Verify configuration
    auto config = w5500.getNetworkConfig();
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mac, config.mac, 6);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80164, config.ip);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFF00, config.subnet);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80101, config.gateway);
    TEST_ASSERT_FALSE(config.dhcp);
}

void test_power_monitoring() {
    // Configure power monitoring pin (GPIO27)
    gpio.pinMode(27, MockGPIO::INPUT_PULLUP_PIN);
    
    // Set up interrupt handler
    bool powerLost = false;
    gpio.attachInterrupt(27, [&powerLost]() {
        powerLost = true;
    }, MockGPIO::FALLING_EDGE);
    
    // Initial state should be HIGH (power present)
    TEST_ASSERT_EQUAL(HIGH, gpio.digitalRead(27));
    TEST_ASSERT_FALSE(powerLost);
    
    // Simulate power loss
    gpio.simulateInterrupt(27, LOW);
    TEST_ASSERT_EQUAL(LOW, gpio.digitalRead(27));
    TEST_ASSERT_TRUE(powerLost);
    
    // Simulate power restoration
    powerLost = false;
    gpio.simulateInterrupt(27, HIGH);
    TEST_ASSERT_EQUAL(HIGH, gpio.digitalRead(27));
    TEST_ASSERT_FALSE(powerLost);
}

void test_flash_storage() {
    // Test data
    const uint8_t testData[] = "SNMP Agent Configuration";
    const size_t testSize = sizeof(testData);
    const uint32_t testAddress = 0x1000;  // Use second sector
    
    // Erase sector before writing
    TEST_ASSERT_TRUE(flash.eraseSector(1));
    TEST_ASSERT_TRUE(flash.isErased(testAddress, testSize));
    
    // Write test data
    TEST_ASSERT_TRUE(flash.write(testAddress, testData, testSize));
    
    // Read back and verify
    uint8_t readBuffer[32];
    TEST_ASSERT_TRUE(flash.read(testAddress, readBuffer, testSize));
    TEST_ASSERT_EQUAL_MEMORY(testData, readBuffer, testSize);
    
    // Test wear leveling
    size_t minErases, maxErases;
    float avgErases;
    flash.getWearStats(minErases, maxErases, avgErases);
    TEST_ASSERT_EQUAL(1, maxErases);  // Only one sector erased
    TEST_ASSERT_EQUAL(0, minErases);  // Other sectors untouched
}

void test_combined_operations() {
    // Set up network
    uint8_t mac[] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};
    w5500.begin(mac);
    w5500.setDHCP(false);
    w5500.setStaticIP(0xC0A80164, 0xFFFFFF00, 0xC0A80101);
    
    // Set up power monitoring
    gpio.pinMode(27, MockGPIO::INPUT_PULLUP_PIN);
    bool powerLost = false;
    gpio.attachInterrupt(27, [&powerLost]() {
        powerLost = true;
    }, MockGPIO::FALLING_EDGE);
    
    // Create SNMP packet
    std::vector<uint32_t> oid = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    std::vector<uint8_t> packet = TestUtils::createSNMPGetRequest("public", oid);
    
    // Open UDP socket and send packet
    TEST_ASSERT_TRUE(w5500.openSocket(0, 161));
    TEST_ASSERT_TRUE(w5500.sendPacket(0, packet.data(), packet.size(), 0xC0A80165, 161));
    
    // Simulate power loss during operation
    gpio.simulateInterrupt(27, LOW);
    TEST_ASSERT_TRUE(powerLost);
    
    // Save state to flash
    const uint32_t stateAddress = 0x1000;
    TEST_ASSERT_TRUE(flash.eraseSector(1));
    uint8_t state = 0x01;  // Power lost state
    TEST_ASSERT_TRUE(flash.write(stateAddress, &state, 1));
    
    // Verify memory usage
    size_t memUsed = TestUtils::getMemoryUsage();
    TEST_ASSERT_LESS_THAN(135168, memUsed);  // Under 50% RAM limit
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_network_configuration);
    RUN_TEST(test_power_monitoring);
    RUN_TEST(test_flash_storage);
    RUN_TEST(test_combined_operations);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
