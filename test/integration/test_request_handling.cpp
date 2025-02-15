#include <unity.h>
#include <Arduino.h>
#include "SNMPMessage.h"
#include "MIB.h"
#include "ASN1Types.h"
#include "../unit/test_mib/test_wrapper.h"

using namespace ASN1;
using namespace SNMP;

void setUp(void) {
    // Setup code if needed before each test
}

void tearDown(void) {
    // Cleanup code if needed after each test
}

void test_get_request_flow() {
    // Initialize MIB and add test data
    MIB mib;
    mib.initialize();
    
    std::vector<uint32_t> sysDescrOID = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    mib.addNode(sysDescrOID, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    
    ASN1::OctetString* value = new ASN1::OctetString("SNMP Pico Power Monitor");
    ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(value);
    mib.setValue(sysDescrOID, wrapper);
    
    // Create GetRequest message
    Message request = Message::createGetRequest("public", ObjectIdentifier(sysDescrOID.data(), sysDescrOID.size()));
    
    // Encode request
    uint8_t requestBuffer[256];
    size_t requestLength = request.encode(requestBuffer, sizeof(requestBuffer));
    TEST_ASSERT_TRUE(requestLength > 0);
    
    // Decode request
    Message decodedRequest;
    bool result = decodedRequest.decode(requestBuffer, requestLength);
    TEST_ASSERT_TRUE(result);
    
    // Process request and generate response
    Message response;
    result = decodedRequest.processRequest(mib, response);
    TEST_ASSERT_TRUE(result);
    
    // Encode response
    uint8_t responseBuffer[256];
    size_t responseLength = response.encode(responseBuffer, sizeof(responseBuffer));
    TEST_ASSERT_TRUE(responseLength > 0);
    
    // Decode and verify response
    Message decodedResponse;
    result = decodedResponse.decode(responseBuffer, responseLength);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(decodedResponse.validateCommunity("public"));
}

void test_get_next_request_flow() {
    // Initialize MIB and add test data
    MIB mib;
    mib.initialize();
    
    // Add multiple nodes
    std::vector<uint32_t> sysDescrOID = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    std::vector<uint32_t> sysObjectIDOID = {1, 3, 6, 1, 2, 1, 1, 2, 0}; // sysObjectID.0
    
    mib.addNode(sysDescrOID, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    mib.addNode(sysObjectIDOID, "sysObjectID", NodeType::SCALAR, AccessType::READ_ONLY);
    
    ASN1::OctetString* value1 = new ASN1::OctetString("SNMP Pico Power Monitor");
    ASN1TypeWrapper* wrapper1 = new ASN1TypeWrapper(value1);
    mib.setValue(sysDescrOID, wrapper1);
    
    ASN1::ObjectIdentifier* value2 = new ASN1::ObjectIdentifier(sysObjectIDOID.data(), sysObjectIDOID.size() - 1);
    ASN1TypeWrapper* wrapper2 = new ASN1TypeWrapper(value2);
    mib.setValue(sysObjectIDOID, wrapper2);
    
    // Create GetNextRequest message for sysDescr (without .0)
    std::vector<uint32_t> startOID = {1, 3, 6, 1, 2, 1, 1, 1}; // sysDescr without .0
    Message request = Message::createGetNextRequest("public", ObjectIdentifier(startOID.data(), startOID.size()));
    
    // Encode request
    uint8_t requestBuffer[256];
    size_t requestLength = request.encode(requestBuffer, sizeof(requestBuffer));
    TEST_ASSERT_TRUE(requestLength > 0);
    
    // Decode request
    Message decodedRequest;
    bool result = decodedRequest.decode(requestBuffer, requestLength);
    TEST_ASSERT_TRUE(result);
    
    // Process request and generate response
    Message response;
    result = decodedRequest.processRequest(mib, response);
    TEST_ASSERT_TRUE(result);
    
    // Encode response
    uint8_t responseBuffer[256];
    size_t responseLength = response.encode(responseBuffer, sizeof(responseBuffer));
    TEST_ASSERT_TRUE(responseLength > 0);
    
    // Decode and verify response
    Message decodedResponse;
    result = decodedResponse.decode(responseBuffer, responseLength);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(decodedResponse.validateCommunity("public"));
}

void test_error_handling() {
    // Initialize MIB
    MIB mib;
    mib.initialize();
    
    // Create GetRequest for non-existent OID
    std::vector<uint32_t> invalidOID = {1, 3, 6, 1, 99, 99, 99, 0};
    Message request = Message::createGetRequest("public", ObjectIdentifier(invalidOID.data(), invalidOID.size()));
    
    // Encode request
    uint8_t requestBuffer[256];
    size_t requestLength = request.encode(requestBuffer, sizeof(requestBuffer));
    TEST_ASSERT_TRUE(requestLength > 0);
    
    // Decode request
    Message decodedRequest;
    bool result = decodedRequest.decode(requestBuffer, requestLength);
    TEST_ASSERT_TRUE(result);
    
    // Process request and generate response
    Message response;
    result = decodedRequest.processRequest(mib, response);
    TEST_ASSERT_TRUE(result);
    
    // Encode response
    uint8_t responseBuffer[256];
    size_t responseLength = response.encode(responseBuffer, sizeof(responseBuffer));
    TEST_ASSERT_TRUE(responseLength > 0);
    
    // Decode and verify error response
    Message decodedResponse;
    result = decodedResponse.decode(responseBuffer, responseLength);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(decodedResponse.validateCommunity("public"));
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_get_request_flow);
    RUN_TEST(test_get_next_request_flow);
    RUN_TEST(test_error_handling);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
