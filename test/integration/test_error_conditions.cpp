#include <unity.h>
#include <Arduino.h>
#include "SNMPMessage.h"
#include "MIB.h"
#include "ASN1Types.h"
#include "../unit/test_mib/test_wrapper.h"

using namespace ASN1;
using namespace SNMP;

// Global variables for rate limiting tests
static unsigned long lastRequestTime = 0;
static int requestCount = 0;

void setUp(void) {
    // Setup code if needed before each test
}

void tearDown(void) {
    // Cleanup code if needed after each test
}

void test_malformed_packets() {
    MIB mib;
    mib.initialize();
    
    // Test 1: Invalid version number
    uint8_t invalidVersion[] = {
        0x30, 0x26,             // SEQUENCE
        0x02, 0x01, 0x02,       // Version (2 - invalid for SNMPv1)
        0x04, 0x06, 'p', 'u', 'b', 'l', 'i', 'c',  // Community string
        0xa0, 0x19              // GetRequest PDU (truncated)
    };
    
    Message msg;
    bool result = msg.decode(invalidVersion, sizeof(invalidVersion));
    TEST_ASSERT_FALSE(result);
    
    // Test 2: Invalid length field
    uint8_t invalidLength[] = {
        0x30, 0xFF,             // SEQUENCE with invalid length
        0x02, 0x01, 0x00,       // Version
        0x04, 0x06, 'p', 'u', 'b', 'l', 'i', 'c'  // Community string
    };
    
    result = msg.decode(invalidLength, sizeof(invalidLength));
    TEST_ASSERT_FALSE(result);
    
    // Test 3: Invalid PDU type
    uint8_t invalidPDU[] = {
        0x30, 0x26,             // SEQUENCE
        0x02, 0x01, 0x00,       // Version
        0x04, 0x06, 'p', 'u', 'b', 'l', 'i', 'c',  // Community string
        0xFF, 0x19              // Invalid PDU type
    };
    
    result = msg.decode(invalidPDU, sizeof(invalidPDU));
    TEST_ASSERT_FALSE(result);
}

void test_invalid_community_strings() {
    MIB mib;
    mib.initialize();
    
    // Add a test node
    std::vector<uint32_t> oid = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    mib.addNode(oid, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::OctetString("Test System"));
    mib.setValue(oid, wrapper);
    
    // Test with invalid community string
    Message request = Message::createGetRequest("invalid", ObjectIdentifier(oid.data(), oid.size()));
    
    // Encode request
    uint8_t buffer[256];
    size_t length = request.encode(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(length > 0);
    
    // Decode request
    Message decodedRequest;
    bool result = decodedRequest.decode(buffer, length);
    TEST_ASSERT_TRUE(result);
    
    // Process request
    Message response;
    result = decodedRequest.processRequest(mib, response);
    TEST_ASSERT_FALSE(result); // Should fail due to invalid community
}

void test_rate_limiting() {
    MIB mib;
    mib.initialize();
    
    // Add a test node
    std::vector<uint32_t> oid = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    mib.addNode(oid, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::OctetString("Test System"));
    mib.setValue(oid, wrapper);
    
    // Send requests rapidly
    const int MAX_REQUESTS = 15; // More than the allowed 10 per second
    int successCount = 0;
    
    for (int i = 0; i < MAX_REQUESTS; i++) {
        Message request = Message::createGetRequest("public", ObjectIdentifier(oid.data(), oid.size()));
        
        // Encode request
        uint8_t buffer[256];
        size_t length = request.encode(buffer, sizeof(buffer));
        TEST_ASSERT_TRUE(length > 0);
        
        // Decode request
        Message decodedRequest;
        bool result = decodedRequest.decode(buffer, length);
        TEST_ASSERT_TRUE(result);
        
        // Process request
        Message response;
        result = decodedRequest.processRequest(mib, response);
        
        if (result) successCount++;
        delay(50); // Small delay between requests
    }
    
    // Should have been rate limited to about 10 requests
    TEST_ASSERT_LESS_OR_EQUAL(10, successCount);
}

void test_recovery_mechanisms() {
    MIB mib;
    mib.initialize();
    
    // Test 1: Recovery from truncated message
    uint8_t truncatedMsg[] = {
        0x30, 0x26,             // SEQUENCE
        0x02, 0x01, 0x00        // Version (truncated after this)
    };
    
    Message msg;
    bool result = msg.decode(truncatedMsg, sizeof(truncatedMsg));
    TEST_ASSERT_FALSE(result);
    
    // Test 2: Recovery from corrupted length fields
    uint8_t corruptedLength[] = {
        0x30, 0xFF,             // SEQUENCE with invalid length
        0x02, 0x01, 0x00,       // Version
        0x04, 0xFF, 'p', 'u'    // Community string with invalid length
    };
    
    result = msg.decode(corruptedLength, sizeof(corruptedLength));
    TEST_ASSERT_FALSE(result);
    
    // Test 3: Recovery from invalid PDU structure
    uint8_t invalidStructure[] = {
        0x30, 0x0F,             // SEQUENCE
        0x02, 0x01, 0x00,       // Version
        0x04, 0x06, 'p', 'u', 'b', 'l', 'i', 'c',  // Community string
        0xa0                     // Incomplete PDU
    };
    
    result = msg.decode(invalidStructure, sizeof(invalidStructure));
    TEST_ASSERT_FALSE(result);
    
    // Verify MIB is still functional after error conditions
    std::vector<uint32_t> oid = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    mib.addNode(oid, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::OctetString("Test System"));
    mib.setValue(oid, wrapper);
    
    // Try a valid request
    Message validRequest = Message::createGetRequest("public", ObjectIdentifier(oid.data(), oid.size()));
    uint8_t buffer[256];
    size_t length = validRequest.encode(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(length > 0);
    
    Message decodedRequest;
    result = decodedRequest.decode(buffer, length);
    TEST_ASSERT_TRUE(result);
    
    Message response;
    result = decodedRequest.processRequest(mib, response);
    TEST_ASSERT_TRUE(result);
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_malformed_packets);
    RUN_TEST(test_invalid_community_strings);
    RUN_TEST(test_rate_limiting);
    RUN_TEST(test_recovery_mechanisms);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
