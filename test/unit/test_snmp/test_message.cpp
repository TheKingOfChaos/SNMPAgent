#include <unity.h>
#include <Arduino.h>
#include "SNMPMessage.h"
#include "ASN1Types.h"

using namespace ASN1;
using namespace SNMP;

void setUp(void) {
    // Setup code if needed before each test
}

void tearDown(void) {
    // Cleanup code if needed after each test
}

void test_snmp_message_construction() {
    // Test basic SNMP message construction
    Message msg;
    
    // Create a GetRequest PDU
    PDU pdu(PDUType::GetRequest);
    pdu.requestID = 1234;
    
    // Add a variable binding
    uint32_t oidValues[] = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    ObjectIdentifier oid(oidValues, 9);
    VarBind vb(oid);
    pdu.addVarBind(vb);
    
    // Encode the message
    uint8_t buffer[256];
    size_t length = msg.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    // Decode and verify
    Message decoded;
    bool result = decoded.decode(buffer, length);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(decoded.validateCommunity("public"));
}

void test_community_string_validation() {
    Message msg;
    
    // Test community string validation
    TEST_ASSERT_TRUE(msg.validateCommunity("public"));
    TEST_ASSERT_FALSE(msg.validateCommunity("invalid"));
    
    // Create message with custom community
    Message customMsg = Message::createGetRequest("private", ObjectIdentifier());
    TEST_ASSERT_TRUE(customMsg.validateCommunity("private"));
    TEST_ASSERT_FALSE(customMsg.validateCommunity("public"));
}

void test_version_handling() {
    Message msg;
    uint8_t buffer[256];
    size_t length = msg.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    Message decoded;
    bool result = decoded.decode(buffer, length);
    
    TEST_ASSERT_TRUE(result);
    // Version should always be SNMPv1 (0)
    TEST_ASSERT_EQUAL(SNMP_VERSION_1, 0);
}

void test_error_response() {
    // Create a request message
    uint32_t oidValues[] = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    ObjectIdentifier oid(oidValues, 9);
    Message request = Message::createGetRequest("public", oid);
    
    // Create error response
    Message response = Message::createErrorResponse(request, ErrorStatus::NoSuchName, 1);
    
    uint8_t buffer[256];
    size_t length = response.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    Message decoded;
    bool result = decoded.decode(buffer, length);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(decoded.validateCommunity("public"));
}

void test_get_next_request() {
    // Test GetNextRequest creation and encoding
    uint32_t oidValues[] = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    ObjectIdentifier oid(oidValues, 9);
    
    Message request = Message::createGetNextRequest("public", oid);
    
    uint8_t buffer[512];
    size_t length = request.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    Message decoded;
    bool result = decoded.decode(buffer, length);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(decoded.validateCommunity("public"));
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_snmp_message_construction);
    RUN_TEST(test_community_string_validation);
    RUN_TEST(test_version_handling);
    RUN_TEST(test_error_response);
    RUN_TEST(test_get_next_request);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
