#include "MIB.h"
#include "ErrorHandler.h"
#include <string.h>
#include <stdlib.h>

MIB::MIB() : node_count_(0) {
}

bool MIB::registerNode(const char* oid, NodeType type, Access access,
                      GetterFunction getter, SetterFunction setter) {
    if (!isValidOID(oid) || node_count_ >= MAX_NODES) {
        REPORT_ERROR(ErrorHandler::Severity::WARNING,
                    ErrorHandler::Category::PROTOCOL,
                    0x6001,
                    "Invalid OID format or MIB full");
        return false;
    }
    
    Node node;
    node.type = type;
    node.access = access;
    node.getter = getter;
    node.setter = setter;
    strncpy(node.oid, oid, sizeof(node.oid) - 1);
    node.oid[sizeof(node.oid) - 1] = '\0';
    
    return addNode(node);
}

bool MIB::getValue(const char* oid, ASN1Object& value) const {
    const Node* node = findNode(oid);
    if (!node || node->access == Access::NOT_ACCESSIBLE || !node->getter) {
        return false;
    }
    
    value = node->getter();
    return true;
}

bool MIB::setValue(const char* oid, const ASN1Object& value) {
    Node* node = findNode(oid);
    if (!node || node->access != Access::READ_WRITE || !node->setter) {
        return false;
    }
    
    return node->setter(value);
}

bool MIB::getNextOID(const char* oid, char* nextOid, size_t maxLength) const {
    if (!oid || !nextOid || maxLength == 0) {
        return false;
    }
    
    // Handle empty OID
    if (oid[0] == '\0') {
        if (node_count_ > 0) {
            strncpy(nextOid, nodes_[0].oid, maxLength - 1);
            nextOid[maxLength - 1] = '\0';
            return true;
        }
        return false;
    }
    
    // Find next OID in lexicographical order
    for (size_t i = 0; i < node_count_; i++) {
        if (compareOID(oid, nodes_[i].oid) < 0) {
            strncpy(nextOid, nodes_[i].oid, maxLength - 1);
            nextOid[maxLength - 1] = '\0';
            return true;
        }
    }
    
    return false;
}

bool MIB::isValidOID(const char* oid) const {
    if (!oid || oid[0] == '\0') {
        return false;
    }
    
    const char* ptr = oid;
    bool first = true;
    
    while (*ptr) {
        // Skip dots
        if (*ptr == '.') {
            ptr++;
            continue;
        }
        
        // Parse number
        char* end;
        long num = strtol(ptr, &end, 10);
        if (end == ptr || num < 0) {
            return false;
        }
        
        // First number must be 0-2
        if (first && num > 2) {
            return false;
        }
        first = false;
        
        ptr = end;
    }
    
    return true;
}

void MIB::initialize() {
    initializeSystemGroup();
}

void MIB::initializeSystemGroup() {
    // System group (.1.3.6.1.2.1.1)
    const char* systemPrefix = "1.3.6.1.2.1.1";
    char oidBuffer[MAX_OID_STRING_LENGTH];
    
    // sysDescr (.1.3.6.1.2.1.1.1)
    snprintf(oidBuffer, sizeof(oidBuffer), "%s.1", systemPrefix);
    registerNode(oidBuffer, NodeType::STRING, Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::OCTET_STRING);
            value.setString("SNMP Power Monitor v1.0", strlen("SNMP Power Monitor v1.0"));
            return value;
        });
    
    // sysObjectID (.1.3.6.1.2.1.1.2)
    snprintf(oidBuffer, sizeof(oidBuffer), "%s.2", systemPrefix);
    registerNode(oidBuffer, NodeType::OID, Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::OBJECT_IDENTIFIER);
            uint32_t enterpriseOid[] = {1, 3, 6, 1, 4, 1, 63050, 1};
            value.setOID(enterpriseOid, 8);
            return value;
        });
    
    // sysUpTime (.1.3.6.1.2.1.1.3)
    snprintf(oidBuffer, sizeof(oidBuffer), "%s.3", systemPrefix);
    registerNode(oidBuffer, NodeType::INTEGER, Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(millis() / 10); // Convert to hundredths of a second
            return value;
        });
    
    // sysContact (.1.3.6.1.2.1.1.4)
    snprintf(oidBuffer, sizeof(oidBuffer), "%s.4", systemPrefix);
    registerNode(oidBuffer, NodeType::STRING, Access::READ_WRITE,
        []() {
            ASN1Object value(ASN1Object::Type::OCTET_STRING);
            value.setString("admin@example.com", strlen("admin@example.com"));
            return value;
        },
        [](const ASN1Object& value) {
            // TODO: Store contact in settings
            return true;
        });
    
    // sysName (.1.3.6.1.2.1.1.5)
    snprintf(oidBuffer, sizeof(oidBuffer), "%s.5", systemPrefix);
    registerNode(oidBuffer, NodeType::STRING, Access::READ_WRITE,
        []() {
            ASN1Object value(ASN1Object::Type::OCTET_STRING);
            value.setString("PowerMonitor", strlen("PowerMonitor"));
            return value;
        },
        [](const ASN1Object& value) {
            // TODO: Store name in settings
            return true;
        });
    
    // sysLocation (.1.3.6.1.2.1.1.6)
    snprintf(oidBuffer, sizeof(oidBuffer), "%s.6", systemPrefix);
    registerNode(oidBuffer, NodeType::STRING, Access::READ_WRITE,
        []() {
            ASN1Object value(ASN1Object::Type::OCTET_STRING);
            value.setString("Server Room", strlen("Server Room"));
            return value;
        },
        [](const ASN1Object& value) {
            // TODO: Store location in settings
            return true;
        });
}

int MIB::compareOID(const char* oid1, const char* oid2) {
    const char *p1 = oid1, *p2 = oid2;
    
    while (*p1 && *p2) {
        // Skip dots
        while (*p1 == '.') p1++;
        while (*p2 == '.') p2++;
        
        if (!*p1 || !*p2) break;
        
        // Compare numbers
        char* end1;
        char* end2;
        long num1 = strtol(p1, &end1, 10);
        long num2 = strtol(p2, &end2, 10);
        
        if (num1 != num2) {
            return num1 - num2;
        }
        
        p1 = end1;
        p2 = end2;
    }
    
    // Handle case where one OID is prefix of the other
    if (*p1) return 1;
    if (*p2) return -1;
    return 0;
}

bool MIB::getParentOID(const char* oid, char* parent, size_t maxLength) {
    if (!oid || !parent || maxLength == 0) {
        return false;
    }
    
    const char* lastDot = strrchr(oid, '.');
    if (!lastDot) {
        return false;
    }
    
    size_t length = lastDot - oid;
    if (length >= maxLength) {
        return false;
    }
    
    strncpy(parent, oid, length);
    parent[length] = '\0';
    return true;
}

bool MIB::isChildOID(const char* parent, const char* child) {
    if (!parent || !child) {
        return false;
    }
    
    size_t parentLen = strlen(parent);
    if (strlen(child) <= parentLen) {
        return false;
    }
    
    return strncmp(parent, child, parentLen) == 0 && child[parentLen] == '.';
}

const MIB::Node* MIB::findNode(const char* oid) const {
    for (size_t i = 0; i < node_count_; i++) {
        if (strcmp(nodes_[i].oid, oid) == 0) {
            return &nodes_[i];
        }
    }
    return nullptr;
}

MIB::Node* MIB::findNode(const char* oid) {
    return const_cast<Node*>(const_cast<const MIB*>(this)->findNode(oid));
}

bool MIB::addNode(const Node& node) {
    if (node_count_ >= MAX_NODES) {
        return false;
    }
    
    // Find insertion point to maintain sorted order
    size_t pos = 0;
    while (pos < node_count_ && compareOID(nodes_[pos].oid, node.oid) < 0) {
        pos++;
    }
    
    // Shift existing nodes
    if (pos < node_count_) {
        memmove(&nodes_[pos + 1], &nodes_[pos], (node_count_ - pos) * sizeof(Node));
    }
    
    // Insert new node
    nodes_[pos] = node;
    node_count_++;
    return true;
}

void MIB::sortNodes() {
    // Simple bubble sort since we don't expect too many nodes
    for (size_t i = 0; i < node_count_ - 1; i++) {
        for (size_t j = 0; j < node_count_ - i - 1; j++) {
            if (compareOID(nodes_[j].oid, nodes_[j + 1].oid) > 0) {
                Node temp = nodes_[j];
                nodes_[j] = nodes_[j + 1];
                nodes_[j + 1] = temp;
            }
        }
    }
}
