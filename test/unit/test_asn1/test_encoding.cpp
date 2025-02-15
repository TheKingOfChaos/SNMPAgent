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

void test_integer_encoding() {
    // Test encoding of INTEGER type
    Integer testInt(42);
    uint8_t buffer[10];
    size_t length = testInt.encode(buffer, sizeof(buffer));

    // Expected encoding for INTEGER 42:
    // Tag: 0x02 (INTEGER)
    // Length: 0x01 (1 byte)
    // Value: 0x2A (42 in hex)
    TEST_ASSERT_EQUAL(3, length);
    TEST_ASSERT_EQUAL_HEX8(INTEGER_TAG, buffer[0]); // Tag
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[1]); // Length
    TEST_ASSERT_EQUAL_HEX8(0x2A, buffer[2]); // Value
}

void test_octet_string_encoding() {
    // Test encoding of OCTET STRING type
    const char* testStr = "test";
    OctetString testOctetStr(testStr);
    uint8_t buffer[10];
    size_t length = testOctetStr.encode(buffer, sizeof(buffer));

    // Expected encoding for OCTET STRING "test":
    // Tag: 0x04 (OCTET STRING)
    // Length: 0x04 (4 bytes)
    // Value: 0x74657374 ("test" in hex)
    TEST_ASSERT_EQUAL(6, length);
    TEST_ASSERT_EQUAL_HEX8(OCTET_STRING_TAG, buffer[0]); // Tag
    TEST_ASSERT_EQUAL_HEX8(0x04, buffer[1]); // Length
    TEST_ASSERT_EQUAL_HEX8(0x74, buffer[2]); // 't'
    TEST_ASSERT_EQUAL_HEX8(0x65, buffer[3]); // 'e'
    TEST_ASSERT_EQUAL_HEX8(0x73, buffer[4]); // 's'
    TEST_ASSERT_EQUAL_HEX8(0x74, buffer[5]); // 't'
}

void test_null_encoding() {
    // Test encoding of NULL type
    Null testNull;
    uint8_t buffer[10];
    size_t length = testNull.encode(buffer, sizeof(buffer));

    // Expected encoding for NULL:
    // Tag: 0x05 (NULL)
    // Length: 0x00 (0 bytes)
    TEST_ASSERT_EQUAL(2, length);
    TEST_ASSERT_EQUAL_HEX8(NULL_TAG, buffer[0]); // Tag
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[1]); // Length
}

void test_oid_encoding() {
    // Test encoding of OBJECT IDENTIFIER type
    // Testing OID: 1.3.6.1.2.1 (iso.org.dod.internet.mgmt.mib-2)
    uint32_t oidValues[] = {1, 3, 6, 1, 2, 1};
    ObjectIdentifier testOid(oidValues, 6);
    uint8_t buffer[10];
    size_t length = testOid.encode(buffer, sizeof(buffer));

    // Expected encoding for OID 1.3.6.1.2.1:
    // Tag: 0x06 (OBJECT IDENTIFIER)
    // Length: 0x05 (5 bytes)
    // Value: 0x2B 0x06 0x01 0x02 0x01
    TEST_ASSERT_EQUAL(7, length);
    TEST_ASSERT_EQUAL_HEX8(OBJECT_IDENTIFIER_TAG, buffer[0]); // Tag
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer[1]); // Length
    TEST_ASSERT_EQUAL_HEX8(0x2B, buffer[2]); // 1.3 encoded as 40*1 + 3 = 43 (0x2B)
    TEST_ASSERT_EQUAL_HEX8(0x06, buffer[3]); // 6
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[4]); // 1
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer[5]); // 2
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[6]); // 1
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
    
    RUN_TEST(test_integer_encoding);
    RUN_TEST(test_octet_string_encoding);
    RUN_TEST(test_null_encoding);
    RUN_TEST(test_oid_encoding);
    
    UNITY_END();
}

void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
}
