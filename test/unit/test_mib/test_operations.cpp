#include <unity.h>
#include "test_wrapper.h"
#include "../../unity_config.h"

TestMIB testMib;
ASN1Object testValue;

void setUp(void) {
    testMib.initialize();
}

void tearDown(void) {
    // Reset test data
    testMib.initialize();
}

void test_mib_get_value() {
    // Test getting existing read-only value
    TEST_ASSERT_TRUE(testMib.getValue("1.3.6.1.2.1.1.1", testValue));
    TEST_ASSERT_EQUAL(ASN1Object::Type::OCTET_STRING, testValue.getType());
    TEST_ASSERT_EQUAL_STRING("Test System Description", testValue.getString().c_str());
    
    // Test getting existing read-write value
    TEST_ASSERT_TRUE(testMib.getValue("1.3.6.1.2.1.1.4", testValue));
    TEST_ASSERT_EQUAL(ASN1Object::Type::OCTET_STRING, testValue.getType());
    TEST_ASSERT_EQUAL_STRING("test@example.com", testValue.getString().c_str());
    
    // Test getting non-existent value
    TEST_ASSERT_FALSE(testMib.getValue("1.3.6.1.2.1.1.999", testValue));
}

void test_mib_set_value() {
    // Test setting read-write value
    testValue.setType(ASN1Object::Type::OCTET_STRING);
    testValue.setString("new@example.com");
    TEST_ASSERT_TRUE(testMib.setValue("1.3.6.1.2.1.1.4", testValue));
    
    // Verify new value
    TEST_ASSERT_TRUE(testMib.getValue("1.3.6.1.2.1.1.4", testValue));
    TEST_ASSERT_EQUAL_STRING("new@example.com", testValue.getString().c_str());
    
    // Test setting read-only value
    testValue.setString("New Description");
    TEST_ASSERT_FALSE(testMib.setValue("1.3.6.1.2.1.1.1", testValue));
    
    // Test setting non-existent value
    TEST_ASSERT_FALSE(testMib.setValue("1.3.6.1.2.1.1.999", testValue));
}

void test_mib_get_next_oid() {
    // Test getting first OID
    std::string nextOID = testMib.getNextOID("");
    TEST_ASSERT_EQUAL_STRING("1.3.6.1.2.1.1.1", nextOID.c_str());
    
    // Test getting next OID in sequence
    nextOID = testMib.getNextOID("1.3.6.1.2.1.1.1");
    TEST_ASSERT_EQUAL_STRING("1.3.6.1.2.1.1.2", nextOID.c_str());
    
    // Test getting next OID after last one
    nextOID = testMib.getNextOID("1.3.6.1.4.1.63050.1.3");
    TEST_ASSERT_EQUAL_STRING("", nextOID.c_str());
}

void test_mib_valid_oid() {
    // Test valid OIDs
    TEST_ASSERT_TRUE(testMib.isValidOID("1.3.6.1.2.1.1.1"));
    TEST_ASSERT_TRUE(testMib.isValidOID("1.3.6.1.4.1.63050.1.1"));
    
    // Test invalid OIDs
    TEST_ASSERT_FALSE(testMib.isValidOID(""));
    TEST_ASSERT_FALSE(testMib.isValidOID("1.a.2"));
    TEST_ASSERT_FALSE(testMib.isValidOID("3.1.1")); // First number > 2
    TEST_ASSERT_FALSE(testMib.isValidOID("-1.2.3"));
}

void test_mib_performance() {
    unsigned long startTime = millis();
    
    // Test get operation timing
    for (int i = 0; i < 1000; i++) {
        testMib.getValue("1.3.6.1.2.1.1.1", testValue);
    }
    
    // Test set operation timing
    testValue.setType(ASN1Object::Type::OCTET_STRING);
    testValue.setString("test value");
    for (int i = 0; i < 1000; i++) {
        testMib.setValue("1.3.6.1.2.1.1.4", testValue);
    }
    
    // Test getNext operation timing
    std::string oid = "";
    for (int i = 0; i < 1000; i++) {
        oid = testMib.getNextOID(oid);
        if (oid.empty()) {
            oid = "";
        }
    }
    
    unsigned long endTime = millis();
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(100, endTime - startTime, "Response time exceeded limit");
}

void setup() {
    UNITY_BEGIN();
    
    RUN_TEST(test_mib_get_value);
    RUN_TEST(test_mib_set_value);
    RUN_TEST(test_mib_get_next_oid);
    RUN_TEST(test_mib_valid_oid);
    RUN_TEST(test_mib_performance);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
