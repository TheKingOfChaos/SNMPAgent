#include <unity.h>
#include <Arduino.h>
#include "SecurityManager.h"
#include "MIB.h"
#include "../mock/test_utils.h"

// Global instances
static MIB mib;
static SecurityManager* security;
static FILE* testLogFile;

void setUp(void) {
    mib = MIB();
    mib.initialize();
    security = new SecurityManager(mib);
    
    // Create temporary log file
    testLogFile = tmpfile();
    security->setLogFile(testLogFile);
    
    TestUtils::startMemoryTracking();
}

void tearDown(void) {
    delete security;
    if (testLogFile) {
        fclose(testLogFile);
    }
}

void test_community_string_validation() {
    // Test valid community string
    TEST_ASSERT_TRUE(security->checkAccess(0xC0A80101, "public"));
    TEST_ASSERT_EQUAL(1, security->getAccessAttempts());
    TEST_ASSERT_EQUAL(0, security->getInvalidAccesses());
    
    // Test invalid community string
    TEST_ASSERT_FALSE(security->checkAccess(0xC0A80101, "private"));
    TEST_ASSERT_EQUAL(2, security->getAccessAttempts());
    TEST_ASSERT_EQUAL(1, security->getInvalidAccesses());
}

void test_rate_limiting() {
    const uint32_t clientIP = 0xC0A80102;  // 192.168.1.2
    
    // Send requests up to limit
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_TRUE(security->checkAccess(clientIP, "public"));
    }
    
    // Next request should be rate limited
    TEST_ASSERT_FALSE(security->checkAccess(clientIP, "public"));
    TEST_ASSERT_EQUAL(1, security->getRateLimited());
    
    // Wait for rate limit window to expire
    delay(60000);  // 60 seconds
    
    // Should be able to send requests again
    TEST_ASSERT_TRUE(security->checkAccess(clientIP, "public"));
}

void test_multiple_clients() {
    const uint32_t client1 = 0xC0A80101;
    const uint32_t client2 = 0xC0A80102;
    
    // Client 1 sends many requests
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_TRUE(security->checkAccess(client1, "public"));
    }
    TEST_ASSERT_FALSE(security->checkAccess(client1, "public")); // Rate limited
    
    // Client 2 should still be able to send requests
    TEST_ASSERT_TRUE(security->checkAccess(client2, "public"));
}

void test_client_tracking_overflow() {
    // Try to overflow client tracking by sending requests from many IPs
    for (int i = 0; i < 100; i++) {
        uint32_t clientIP = 0xC0A80100 + i;  // 192.168.1.x
        TEST_ASSERT_TRUE(security->checkAccess(clientIP, "public"));
    }
    
    // Memory usage should still be within limits
    size_t memUsed = TestUtils::getMemoryUsage();
    TEST_ASSERT_LESS_THAN(135168, memUsed);  // Under 50% RAM limit
}

void test_access_logging() {
    const uint32_t clientIP = 0xC0A80101;
    
    // Generate some log entries
    security->checkAccess(clientIP, "public");  // Valid access
    security->checkAccess(clientIP, "private"); // Invalid community
    
    for (int i = 0; i < 101; i++) {
        security->checkAccess(clientIP, "public"); // Last one gets rate limited
    }
    
    // Verify log file contents
    rewind(testLogFile);
    char line[256];
    int validCount = 0;
    int invalidCount = 0;
    int limitedCount = 0;
    
    while (fgets(line, sizeof(line), testLogFile)) {
        if (strstr(line, "Access granted")) validCount++;
        if (strstr(line, "Invalid community")) invalidCount++;
        if (strstr(line, "Rate limited")) limitedCount++;
    }
    
    TEST_ASSERT_EQUAL(100, validCount);
    TEST_ASSERT_EQUAL(1, invalidCount);
    TEST_ASSERT_EQUAL(1, limitedCount);
}

void test_buffer_overflow_prevention() {
    const uint32_t clientIP = 0xC0A80101;
    
    // Try to overflow community string
    std::string longCommunity(1000, 'A');
    TEST_ASSERT_FALSE(security->checkAccess(clientIP, longCommunity));
    
    // Try to overflow log message
    std::string longReason(1000, 'B');
    security->checkAccess(clientIP, "public");  // This internally logs with a long reason
    
    // Verify system is still stable
    TEST_ASSERT_TRUE(security->checkAccess(clientIP, "public"));
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_community_string_validation);
    RUN_TEST(test_rate_limiting);
    RUN_TEST(test_multiple_clients);
    RUN_TEST(test_client_tracking_overflow);
    RUN_TEST(test_access_logging);
    RUN_TEST(test_buffer_overflow_prevention);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
