#ifndef MIB_H
#define MIB_H

#include "ASN1Object.h"
#include <cstddef>

class MIB {
public:
    // MIB node types
    enum class NodeType {
        INTEGER,
        STRING,
        OID,
        NULL_TYPE,
        SEQUENCE
    };
    
    // MIB node access
    enum class Access {
        READ_ONLY,
        READ_WRITE,
        NOT_ACCESSIBLE
    };
    
    // Function pointer types for getters and setters
    typedef ASN1Object (*GetterFunction)();
    typedef bool (*SetterFunction)(const ASN1Object&);
    
    // MIB node definition
    struct Node {
        NodeType type;
        Access access;
        GetterFunction getter;
        SetterFunction setter;
        char oid[64];  // Store OID as string
    };
    
    static constexpr size_t MAX_NODES = 100;
    static constexpr size_t MAX_OID_STRING_LENGTH = 64;
    
    MIB();
    
    // Node registration
    bool registerNode(const char* oid, NodeType type, Access access,
                     GetterFunction getter, SetterFunction setter = nullptr);
    
    // Node access
    bool getValue(const char* oid, ASN1Object& value) const;
    bool setValue(const char* oid, const ASN1Object& value);
    
    // OID navigation
    bool getNextOID(const char* oid, char* nextOid, size_t maxLength) const;
    bool isValidOID(const char* oid) const;
    
    // MIB initialization
    void initialize();
    
private:
    Node nodes_[MAX_NODES];
    size_t node_count_;
    
    // System group initialization
    void initializeSystemGroup();
    
    // Helper methods
    static int compareOID(const char* oid1, const char* oid2);
    static bool getParentOID(const char* oid, char* parent, size_t maxLength);
    static bool isChildOID(const char* parent, const char* child);
    
    // Node management
    const Node* findNode(const char* oid) const;
    Node* findNode(const char* oid);
    bool addNode(const Node& node);
    void sortNodes();  // Keep nodes sorted by OID for efficient lookup
};

#endif // MIB_H
