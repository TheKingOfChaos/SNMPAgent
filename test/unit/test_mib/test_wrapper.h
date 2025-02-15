#ifndef TEST_MIB_WRAPPER_H
#define TEST_MIB_WRAPPER_H

#include "MIB.h"
#include "ASN1Object.h"
#include <map>
#include <string>

class TestMIB : public MIB {
public:
    // Test data structure
    struct TestNode {
        std::string value;
        bool writable;
        bool exists;
    };
    
    // Test data access
    void setTestData(const std::string& oid, const std::string& value, bool writable = false) {
        testData_[oid] = {value, writable, true};
        
        // Register node in MIB
        NodeType type = NodeType::STRING;
        Access access = writable ? Access::READ_WRITE : Access::READ_ONLY;
        
        auto getter = [this, oid]() {
            ASN1Object value(ASN1Object::Type::OCTET_STRING);
            value.setString(testData_[oid].value);
            return value;
        };
        
        std::function<bool(const ASN1Object&)> setter;
        if (writable) {
            setter = [this, oid](const ASN1Object& value) {
                testData_[oid].value = value.getString();
                return true;
            };
        }
        
        registerNode(oid, type, access, getter, setter);
    }
    
    void removeTestData(const std::string& oid) {
        testData_[oid].exists = false;
    }
    
    const TestNode& getTestNode(const std::string& oid) const {
        static TestNode emptyNode = {"", false, false};
        auto it = testData_.find(oid);
        return (it != testData_.end()) ? it->second : emptyNode;
    }
    
    // Test helper methods
    bool verifyAccess(const std::string& oid, bool write = false) const {
        const TestNode& node = getTestNode(oid);
        return node.exists && (!write || node.writable);
    }
    
    bool verifyValue(const std::string& oid, const std::string& expectedValue) const {
        const TestNode& node = getTestNode(oid);
        return node.exists && node.value == expectedValue;
    }
    
private:
    std::map<std::string, TestNode> testData_;
};

#endif // TEST_MIB_WRAPPER_H
