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

void test_integer_decoding() {
    // Create a buffer with encoded INTEGER (42)
    uint8_t buffer[] = {
        INTEGER_TAG,  // Tag
        0x01,        // Length
        0x2A         // Value (42)
    };
    
    Integer testInt;
    size_t bytesRead = 0;
    bool result = testInt.decode(buffer, sizeof(buffer), bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, bytesRead);
    TEST_ASSERT_EQUAL(42, testInt.getValue());
}

void test_octet_string_decoding() {
    // Create a buffer with encoded OCTET STRING ("test")
    uint8_t buffer[] = {
        OCTET_STRING_TAG,  // Tag
        0x04,             // Length
        0x74, 0x65, 0x73, 0x74  // Value ("test")
    };
    
    OctetString testStr;
    size_t bytesRead = 0;
    bool result = testStr.decode(buffer, sizeof(buffer), bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(6, bytesRead);
    TEST_ASSERT_EQUAL_STRING("test", testStr.getValue());
}

void test_null_decoding() {
    // Create a buffer with encoded NULL
    uint8_t buffer[] = {
        NULL_TAG,  // Tag
        0x00      // Length
    };
    
    Null testNull;
    size_t bytesRead = 0;
    bool result = testNull.decode(buffer, sizeof(buffer), bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, bytesRead);
}

void test_oid_decoding() {
    // Create a buffer with encoded OID (1.3.6.1.2.1)
    uint8_t buffer[] = {
        OBJECT_IDENTIFIER_TAG,  // Tag
        0x05,                  // Length
        0x2B,                  // 1.3 (40*1 + 3)
        0x06,                  // 6
        0x01,                  // 1
        0x02,                  // 2
        0x01                   // 1
    };
    
    ObjectIdentifier testOid;
    size_t bytesRead = 0;
    bool result = testOid.decode(buffer, sizeof(buffer), bytesRead);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(7, bytesRead);
    TEST_ASSERT_EQUAL(6, testOid.getComponentCount());
    
    const uint32_t* components = testOid.getComponents();
    TEST_ASSERT_EQUAL(1, components[0]);
    TEST_ASSERT_EQUAL(3, components[1]);
    TEST_ASSERT_EQUAL(6, components[2]);
    TEST_ASSERT_EQUAL(1, components[3]);
    TEST_ASSERT_EQUAL(2, components[4]);
    TEST_ASSERT_EQUAL(1, components[5]);
}

void test_invalid_tag_decoding() {
    // Test with invalid tag for INTEGER
    uint8_t buffer[] = {
        0x03,  // Wrong tag (BitString instead of Integer)
        0x01,  // Length
        0x2A   // Value
    };
    
    Integer testInt;
    size_t bytesRead = 0;
    bool result = testInt.decode(buffer, sizeof(buffer), bytesRead);
    
    TEST_ASSERT_FALSE(result);
}

void test_invalid_length_decoding() {
    // Test with invalid length for OCTET STRING
    uint8_t buffer[] = {
        OCTET_STRING_TAG,  // Tag
        0x05,             // Length (too long)
        0x74, 0x65, 0x73, 0x74  // Value ("test")
    };
    
    OctetString testStr;
    size_t bytesRead = 0;
    bool result = testStr.decode(buffer, sizeof(buffer), bytesRead);
    
    TEST_ASSERT_FALSE(result);
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
    
    RUN_TEST(test_integer_decoding);
    RUN_TEST(test_octet_string_decoding);
    RUN_TEST(test_null_decoding);
    RUN_TEST(test_oid_decoding);
    RUN_TEST(test_invalid_tag_decoding);
    RUN_TEST(test_invalid_length_decoding);
    
    UNITY_END();
}

void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
}
