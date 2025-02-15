#include <unity.h>
#include <Arduino.h>
#include "SNMPMessage.h"
#include "MIB.h"
#include "ASN1Types.h"
#include "../unit/test_mib/test_wrapper.h"

using namespace ASN1;
using namespace SNMP;

// Memory tracking variables
// Memory tracking variables
static size_t initialFreeRam;
static size_t peakRamUsage;

extern "C" {
    // RP2040 memory functions
    extern char __heap_start[];
    extern char __heap_end[];
    extern char *__brkval;
}

// Get available RAM
static size_t getFreeRam() {
    char *heap_end = (__brkval == 0 ? __heap_start : __brkval);
    return __heap_end - heap_end;
}

void setUp(void) {
    initialFreeRam = getFreeRam();
    peakRamUsage = 0;
}

void tearDown(void) {
    size_t currentRam = getFreeRam();
    size_t ramUsed = initialFreeRam - currentRam;
    if (ramUsed > peakRamUsage) {
        peakRamUsage = ramUsed;
    }
}

void test_response_timing() {
    MIB mib;
    mib.initialize();
    
    // Add test nodes
    std::vector<uint32_t> oid = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    mib.addNode(oid, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::OctetString("Test System"));
    mib.setValue(oid, wrapper);
    
    // Create request
    Message request = Message::createGetRequest("public", ObjectIdentifier(oid.data(), oid.size()));
    uint8_t buffer[256];
    size_t length = request.encode(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(length > 0);
    
    // Measure response time
    unsigned long startTime = micros();
    
    // Process request
    Message decodedRequest;
    bool result = decodedRequest.decode(buffer, length);
    TEST_ASSERT_TRUE(result);
    
    Message response;
    result = decodedRequest.processRequest(mib, response);
    TEST_ASSERT_TRUE(result);
    
    // Encode response
    length = response.encode(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(length > 0);
    
    unsigned long duration = micros() - startTime;
    
    // Response should be under 100ms (100,000 microseconds)
    TEST_ASSERT_LESS_THAN(100000, duration);
}

void test_memory_usage() {
    MIB mib;
    mib.initialize();
    
    // Initial memory state
    size_t startRam = getFreeRam();
    
    // Add multiple nodes and perform operations
    for (int i = 1; i <= 50; i++) {
        std::vector<uint32_t> oid = {1, 3, 6, 1, 4, 1, 63050, 1, (uint32_t)i};
        mib.addNode(oid, "testNode" + std::to_string(i), NodeType::SCALAR, AccessType::READ_ONLY);
        
        ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::Integer(i));
        mib.setValue(oid, wrapper);
        
        // Check memory after each operation
        size_t currentRam = getFreeRam();
        size_t ramUsed = startRam - currentRam;
        
        // Memory usage should stay under 50% of total RAM (264KB = 270336 bytes)
        TEST_ASSERT_LESS_THAN(135168, ramUsed); // 50% of 264KB
        
        if (ramUsed > peakRamUsage) {
            peakRamUsage = ramUsed;
        }
    }
    
    // Verify memory recovery after cleanup
    for (int i = 1; i <= 50; i++) {
        std::vector<uint32_t> oid = {1, 3, 6, 1, 4, 1, 63050, 1, (uint32_t)i};
        // Note: Memory will be freed by MIB destructor
    }
    
    size_t endRam = getFreeRam();
    size_t finalRamUsed = startRam - endRam;
    
    // Most memory should be recovered
    TEST_ASSERT_LESS_THAN(1024, finalRamUsed); // Allow for some small overhead
}

void test_rate_limiting_performance() {
    MIB mib;
    mib.initialize();
    
    std::vector<uint32_t> oid = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    mib.addNode(oid, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::OctetString("Test System"));
    mib.setValue(oid, wrapper);
    
    // Test sustained request handling
    const int TEST_DURATION_MS = 5000; // 5 seconds
    const int EXPECTED_REQUESTS = 50; // 10 requests per second * 5 seconds
    int successCount = 0;
    int totalRequests = 0;
    
    unsigned long startTime = millis();
    unsigned long lastRequestTime = 0;
    
    while (millis() - startTime < TEST_DURATION_MS) {
        // Ensure minimum spacing between requests
        if (millis() - lastRequestTime < 20) { // 20ms minimum spacing
            continue;
        }
        
        Message request = Message::createGetRequest("public", ObjectIdentifier(oid.data(), oid.size()));
        uint8_t buffer[256];
        size_t length = request.encode(buffer, sizeof(buffer));
        
        Message decodedRequest;
        bool result = decodedRequest.decode(buffer, length);
        TEST_ASSERT_TRUE(result);
        
        Message response;
        result = decodedRequest.processRequest(mib, response);
        
        if (result) {
            successCount++;
            
            // Verify response time for successful requests
            length = response.encode(buffer, sizeof(buffer));
            TEST_ASSERT_TRUE(length > 0);
        }
        
        totalRequests++;
        lastRequestTime = millis();
        
        // Monitor memory usage during sustained operation
        size_t currentRam = getFreeRam();
        size_t ramUsed = initialFreeRam - currentRam;
        if (ramUsed > peakRamUsage) {
            peakRamUsage = ramUsed;
        }
    }
    
    // Verify rate limiting effectiveness
    TEST_ASSERT_INT_WITHIN(5, EXPECTED_REQUESTS, successCount); // Allow small variance
    
    // Verify system stability
    TEST_ASSERT_LESS_THAN(135168, peakRamUsage); // Memory usage stayed under 50%
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_response_timing);
    RUN_TEST(test_memory_usage);
    RUN_TEST(test_rate_limiting_performance);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
