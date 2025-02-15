#include <unity.h>
#include <Arduino.h>
#include "ASN1Types.h"

using namespace ASN1;

void setUp(void) {
    // Setup code if needed before each test
}

void tearDown(void) {
    // Cleanup code if needed after each test
}

void test_integer_bounds() {
    // Test maximum integer value
    Integer maxInt(INT32_MAX);
    uint8_t buffer[MAX_INT_LENGTH + 2]; // +2 for tag and length
    size_t length = maxInt.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    Integer decoded;
    size_t bytesRead = 0;
    bool result = decoded.decode(buffer, length, bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(INT32_MAX, decoded.getValue());
    
    // Test minimum integer value
    Integer minInt(INT32_MIN);
    length = minInt.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    result = decoded.decode(buffer, length, bytesRead);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(INT32_MIN, decoded.getValue());
}

void test_octet_string_max_length() {
    // Create string of maximum allowed length
    char maxStr[MAX_STRING_LENGTH];
    memset(maxStr, 'A', MAX_STRING_LENGTH - 1);
    maxStr[MAX_STRING_LENGTH - 1] = '\0';
    
    OctetString testStr(maxStr);
    uint8_t buffer[MAX_STRING_LENGTH + 4]; // +4 for tag, length bytes (might need multiple length bytes)
    size_t length = testStr.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    OctetString decoded;
    size_t bytesRead = 0;
    bool result = decoded.decode(buffer, length, bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING(maxStr, decoded.getValue());
}

void test_oid_max_components() {
    // Create OID with maximum number of components
    uint32_t components[MAX_OID_LENGTH];
    for (size_t i = 0; i < MAX_OID_LENGTH; i++) {
        components[i] = i + 1;
    }
    
    ObjectIdentifier testOid(components, MAX_OID_LENGTH);
    uint8_t buffer[MAX_OID_LENGTH * 5]; // Worst case: 5 bytes per component
    size_t length = testOid.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(length > 0);
    
    ObjectIdentifier decoded;
    size_t bytesRead = 0;
    bool result = decoded.decode(buffer, length, bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(MAX_OID_LENGTH, decoded.getComponentCount());
    
    const uint32_t* decodedComponents = decoded.getComponents();
    for (size_t i = 0; i < MAX_OID_LENGTH; i++) {
        TEST_ASSERT_EQUAL(components[i], decodedComponents[i]);
    }
}

void test_buffer_overflow_protection() {
    // Test encoding with insufficient buffer size
    Integer testInt(42);
    uint8_t smallBuffer[2]; // Too small for INTEGER encoding (needs 3 bytes)
    size_t length = testInt.encode(smallBuffer, sizeof(smallBuffer));
    
    TEST_ASSERT_EQUAL(0, length); // Should return 0 to indicate failure
    
    // Test decoding with insufficient buffer size
    uint8_t validBuffer[] = {INTEGER_TAG, 0x01, 0x2A};
    Integer decoded;
    size_t bytesRead = 0;
    bool result = decoded.decode(validBuffer, 2, bytesRead); // Pass smaller size than actual buffer
    
    TEST_ASSERT_FALSE(result); // Should fail due to insufficient data
}

void test_invalid_oid_components() {
    // Test OID with invalid first/second components
    uint32_t invalidComponents[] = {3, 40, 1, 2, 3}; // Invalid: first component > 2
    ObjectIdentifier testOid(invalidComponents, 5);
    uint8_t buffer[32];
    size_t length = testOid.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL(0, length); // Should fail to encode
    
    // Test OID with second component >= 40 when first component < 2
    uint32_t invalidComponents2[] = {1, 40, 1, 2, 3}; // Invalid: second component >= 40 with first component 1
    ObjectIdentifier testOid2(invalidComponents2, 5);
    length = testOid2.encode(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL(0, length); // Should fail to encode
}

void test_sequence_validation() {
    // Test SEQUENCE with invalid length
    uint8_t invalidBuffer[] = {
        SEQUENCE_TAG,
        0x04,           // Length claims 4 bytes
        0x02, 0x01      // Only 2 bytes provided
    };
    
    Sequence testSeq;
    size_t bytesRead = 0;
    bool result = testSeq.decode(invalidBuffer, sizeof(invalidBuffer), bytesRead);
    
    TEST_ASSERT_FALSE(result); // Should fail due to length mismatch
}

void setup() {
    // Initialize LED pin
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    // Wait for board to settle and serial to be ready
    delay(2000);
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    UNITY_BEGIN();
    
    RUN_TEST(test_integer_bounds);
    RUN_TEST(test_octet_string_max_length);
    RUN_TEST(test_oid_max_components);
    RUN_TEST(test_buffer_overflow_protection);
    RUN_TEST(test_invalid_oid_components);
    RUN_TEST(test_sequence_validation);
    
    UNITY_END();
}

void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
}
