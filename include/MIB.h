#ifndef MIB_H
#define MIB_H

#include <vector>
#include <memory>
#include <string>
#include <functional>

// Forward declarations
class ASN1Object;

// Forward declarations
class MIBNode;
using MIBNodePtr = std::shared_ptr<MIBNode>;

// Function type for getting dynamic values
using ValueCallback = std::function<ASN1Object*()>;

// MIB Node Access Types
enum class AccessType {
    READ_ONLY,
    READ_WRITE,
    NOT_ACCESSIBLE
};

// MIB Node Types
enum class NodeType {
    SCALAR,     // Single value
    TABLE,      // Table container
    ROW,        // Table row
    COLUMN,     // Table column
    CONTAINER   // Just for grouping, no value
};

// Forward declare MIB for friendship
class MIB;

// MIB Node class representing a node in the OID tree
class MIBNode : public std::enable_shared_from_this<MIBNode> {
    friend class MIB;  // Allow MIB to access private members
public:
    MIBNode(const std::string& name, uint32_t id, NodeType type = NodeType::SCALAR,
            AccessType access = AccessType::READ_ONLY);
    
    // Add a child node
    void addChild(MIBNodePtr child);
    
    // Set static value
    void setValue(ASN1Object* value);
    
    // Set dynamic value callback
    void setCallback(ValueCallback callback);
    
    // Get the current value (either static or from callback)
    ASN1Object* getValue() const;
    
    // Navigation methods
    MIBNodePtr getChild(uint32_t id) const;
    MIBNodePtr getNextSibling() const;
    MIBNodePtr getParent() const;
    
    // OID methods
    std::vector<uint32_t> getOID() const;
    bool matchOID(const std::vector<uint32_t>& oid) const;
    
    // Getters
    const std::string& getName() const { return name_; }
    uint32_t getID() const { return id_; }
    NodeType getType() const { return type_; }
    AccessType getAccess() const { return access_; }
    
private:
    std::string name_;                    // Node name (for display/debug)
    uint32_t id_;                         // Node ID in OID
    NodeType type_;                       // Node type
    AccessType access_;                   // Access type
    std::weak_ptr<MIBNode> parent_;       // Parent node
    std::vector<MIBNodePtr> children_;    // Child nodes
    ASN1Object* staticValue_;             // Static value if any
    ValueCallback valueCallback_;         // Callback for dynamic values
};

// MIB class managing the entire MIB structure
class MIB {
public:
    MIB();
    
    // Initialize the MIB tree with standard nodes
    void initialize();
    
    // Add a node at a specific OID path
    void addNode(const std::vector<uint32_t>& oid, const std::string& name,
                NodeType type = NodeType::SCALAR,
                AccessType access = AccessType::READ_ONLY);
    
    // Set a value at a specific OID
    void setValue(const std::vector<uint32_t>& oid, ASN1Object* value);
    
    // Set a callback for dynamic value at a specific OID
    void setCallback(const std::vector<uint32_t>& oid, ValueCallback callback);
    
    // Get value at a specific OID
    ASN1Object* getValue(const std::vector<uint32_t>& oid) const;
    
    // Get the next OID in sequence (for GetNext operations)
    std::vector<uint32_t> getNextOID(const std::vector<uint32_t>& oid) const;
    
    // Find a node by OID
    MIBNodePtr findNode(const std::vector<uint32_t>& oid) const;
    
private:
    MIBNodePtr root_;  // Root of the MIB tree
    
    // Helper method to create standard OID structure
    void createStandardOIDStructure();
    
    // Helper method to create system group
    void createSystemGroup();
    
    // Helper method to create private group
    void createPrivateGroup();
};

#endif // MIB_H
