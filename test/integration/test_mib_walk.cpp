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

void test_sequential_oid_retrieval() {
    // Initialize MIB and add test data
    MIB mib;
    mib.initialize();
    
    // Add system group nodes
    std::vector<uint32_t> sysDescrOID = {1, 3, 6, 1, 2, 1, 1, 1, 0}; // sysDescr.0
    std::vector<uint32_t> sysObjectIDOID = {1, 3, 6, 1, 2, 1, 1, 2, 0}; // sysObjectID.0
    std::vector<uint32_t> sysNameOID = {1, 3, 6, 1, 2, 1, 1, 5, 0}; // sysName.0
    std::vector<uint32_t> sysLocationOID = {1, 3, 6, 1, 2, 1, 1, 6, 0}; // sysLocation.0
    
    mib.addNode(sysDescrOID, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    mib.addNode(sysObjectIDOID, "sysObjectID", NodeType::SCALAR, AccessType::READ_ONLY);
    mib.addNode(sysNameOID, "sysName", NodeType::SCALAR, AccessType::READ_WRITE);
    mib.addNode(sysLocationOID, "sysLocation", NodeType::SCALAR, AccessType::READ_WRITE);
    
    // Set values
    ASN1TypeWrapper* wrapper1 = new ASN1TypeWrapper(new ASN1::OctetString("SNMP Pico Power Monitor"));
    ASN1TypeWrapper* wrapper2 = new ASN1TypeWrapper(new ASN1::ObjectIdentifier(sysObjectIDOID.data(), sysObjectIDOID.size() - 1));
    ASN1TypeWrapper* wrapper3 = new ASN1TypeWrapper(new ASN1::OctetString("PowerMon01"));
    ASN1TypeWrapper* wrapper4 = new ASN1TypeWrapper(new ASN1::OctetString("Server Room"));
    
    mib.setValue(sysDescrOID, wrapper1);
    mib.setValue(sysObjectIDOID, wrapper2);
    mib.setValue(sysNameOID, wrapper3);
    mib.setValue(sysLocationOID, wrapper4);
    
    // Start walk from system group root
    std::vector<uint32_t> startOID = {1, 3, 6, 1, 2, 1, 1}; // system group
    std::vector<uint32_t> currentOID = startOID;
    
    // Walk through all OIDs
    int nodeCount = 0;
    do {
        currentOID = mib.getNextOID(currentOID);
        if (currentOID.empty()) break;
        
        ASN1Object* value = mib.getValue(currentOID);
        TEST_ASSERT_NOT_NULL(value);
        nodeCount++;
    } while (nodeCount < 10); // Safety limit
    
    // Should have found all 4 nodes
    TEST_ASSERT_EQUAL(4, nodeCount);
}

void test_boundary_conditions() {
    MIB mib;
    mib.initialize();
    
    // Test walk from non-existent OID
    std::vector<uint32_t> invalidOID = {1, 3, 6, 1, 99, 99, 99};
    std::vector<uint32_t> nextOID = mib.getNextOID(invalidOID);
    TEST_ASSERT_TRUE(nextOID.empty());
    
    // Test walk from last OID in tree
    std::vector<uint32_t> lastOID = {1, 3, 6, 1, 4, 1, 63050}; // Our private enterprise number
    nextOID = mib.getNextOID(lastOID);
    TEST_ASSERT_TRUE(nextOID.empty());
}

void test_large_mib_traversal() {
    MIB mib;
    mib.initialize();
    
    // Add a large number of test nodes
    for (uint32_t i = 1; i <= 100; i++) {
        std::vector<uint32_t> oid = {1, 3, 6, 1, 4, 1, 63050, 1, i};
        std::string name = "testNode" + std::to_string(i);
        mib.addNode(oid, name, NodeType::SCALAR, AccessType::READ_ONLY);
        
        ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::Integer(i));
        mib.setValue(oid, wrapper);
    }
    
    // Walk through all nodes
    std::vector<uint32_t> startOID = {1, 3, 6, 1, 4, 1, 63050, 1};
    std::vector<uint32_t> currentOID = startOID;
    int nodeCount = 0;
    
    do {
        currentOID = mib.getNextOID(currentOID);
        if (currentOID.empty()) break;
        
        ASN1Object* value = mib.getValue(currentOID);
        TEST_ASSERT_NOT_NULL(value);
        nodeCount++;
    } while (nodeCount < 200); // Safety limit
    
    // Should have found all 100 nodes
    TEST_ASSERT_EQUAL(100, nodeCount);
}

void test_traversal_performance() {
    MIB mib;
    mib.initialize();
    
    // Add test nodes
    for (uint32_t i = 1; i <= 50; i++) {
        std::vector<uint32_t> oid = {1, 3, 6, 1, 4, 1, 63050, 1, i};
        std::string name = "testNode" + std::to_string(i);
        mib.addNode(oid, name, NodeType::SCALAR, AccessType::READ_ONLY);
        
        ASN1TypeWrapper* wrapper = new ASN1TypeWrapper(new ASN1::Integer(i));
        mib.setValue(oid, wrapper);
    }
    
    // Measure time for complete walk
    unsigned long startTime = millis();
    
    std::vector<uint32_t> startOID = {1, 3, 6, 1, 4, 1, 63050, 1};
    std::vector<uint32_t> currentOID = startOID;
    int nodeCount = 0;
    
    do {
        currentOID = mib.getNextOID(currentOID);
        if (currentOID.empty()) break;
        
        ASN1Object* value = mib.getValue(currentOID);
        TEST_ASSERT_NOT_NULL(value);
        nodeCount++;
    } while (nodeCount < 100); // Safety limit
    
    unsigned long duration = millis() - startTime;
    
    // Complete walk should take less than 100ms
    TEST_ASSERT_LESS_THAN(100, duration);
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_sequential_oid_retrieval);
    RUN_TEST(test_boundary_conditions);
    RUN_TEST(test_large_mib_traversal);
    RUN_TEST(test_traversal_performance);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
