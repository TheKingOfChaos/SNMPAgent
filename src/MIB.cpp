#include "MIB.h"
#include "ASN1Types.h"
#include "ASN1Object.h"
#include <algorithm>
#include <sstream>

// MIBNode implementation
MIBNode::MIBNode(const std::string& name, uint32_t id, NodeType type, AccessType access)
    : name_(name), id_(id), type_(type), access_(access), staticValue_(nullptr) {}

void MIBNode::addChild(MIBNodePtr child) {
    children_.push_back(child);
    child->parent_ = std::weak_ptr<MIBNode>(shared_from_this());
    
    // Sort children by ID for proper GetNext operation
    std::sort(children_.begin(), children_.end(),
              [](const MIBNodePtr& a, const MIBNodePtr& b) {
                  return a->getID() < b->getID();
              });
}

void MIBNode::setValue(ASN1Object* value) {
    if (staticValue_) {
        delete staticValue_;
    }
    staticValue_ = value;
    valueCallback_ = nullptr; // Clear any existing callback
}

void MIBNode::setCallback(ValueCallback callback) {
    valueCallback_ = callback;
    if (staticValue_) {
        delete staticValue_;
        staticValue_ = nullptr;
    }
}

ASN1Object* MIBNode::getValue() const {
    if (valueCallback_) {
        return valueCallback_();
    }
    return staticValue_;
}

MIBNodePtr MIBNode::getChild(uint32_t id) const {
    auto it = std::find_if(children_.begin(), children_.end(),
                          [id](const MIBNodePtr& child) {
                              return child->getID() == id;
                          });
    return (it != children_.end()) ? *it : nullptr;
}

MIBNodePtr MIBNode::getNextSibling() const {
    if (auto parent = parent_.lock()) {
        auto& siblings = parent->children_;
        auto it = std::find_if(siblings.begin(), siblings.end(),
                              [this](const MIBNodePtr& node) {
                                  return node.get() == this;
                              });
        if (it != siblings.end() && ++it != siblings.end()) {
            return *it;
        }
    }
    return nullptr;
}

MIBNodePtr MIBNode::getParent() const {
    return parent_.lock();
}

std::vector<uint32_t> MIBNode::getOID() const {
    std::vector<uint32_t> oid;
    const MIBNode* node = this;
    
    while (node) {
        oid.insert(oid.begin(), node->id_);
        if (auto parent = node->parent_.lock()) {
            node = parent.get();
        } else {
            break;
        }
    }
    
    return oid;
}

bool MIBNode::matchOID(const std::vector<uint32_t>& oid) const {
    return getOID() == oid;
}

// MIB implementation
MIB::MIB() {
    // Create root node
    root_ = std::make_shared<MIBNode>("root", 1, NodeType::CONTAINER, AccessType::NOT_ACCESSIBLE);
}

void MIB::initialize() {
    createStandardOIDStructure();
    createSystemGroup();
    createPrivateGroup();
}

void MIB::addNode(const std::vector<uint32_t>& oid, const std::string& name,
                     NodeType type, AccessType access) {
    if (oid.empty()) return;
    
    MIBNodePtr current = root_;
    for (size_t i = 0; i < oid.size() - 1; ++i) {
        auto child = current->getChild(oid[i]);
        if (!child) {
            // Create intermediate nodes as containers
            child = std::make_shared<MIBNode>("", oid[i], NodeType::CONTAINER, AccessType::NOT_ACCESSIBLE);
            current->addChild(child);
        }
        current = child;
    }
    
    // Create the final node with the specified properties
    auto finalNode = std::make_shared<MIBNode>(name, oid.back(), type, access);
    current->addChild(finalNode);
}

void MIB::setValue(const std::vector<uint32_t>& oid, ASN1Object* value) {
    auto node = findNode(oid);
    if (node) {
        node->setValue(value);
    }
}

void MIB::setCallback(const std::vector<uint32_t>& oid, ValueCallback callback) {
    auto node = findNode(oid);
    if (node) {
        node->setCallback(callback);
    }
}

ASN1Object* MIB::getValue(const std::vector<uint32_t>& oid) const {
    auto node = findNode(oid);
    return node ? node->getValue() : nullptr;
}

std::vector<uint32_t> MIB::getNextOID(const std::vector<uint32_t>& oid) const {
    // Special case for empty OID - return first node
    if (oid.empty() && !root_->children_.empty()) {
        return root_->children_[0]->getOID();
    }
    
    auto node = findNode(oid);
    if (!node) return std::vector<uint32_t>();
    
    // First try children
    if (!node->children_.empty()) {
        return node->children_[0]->getOID();
    }
    
    // Then try siblings
    while (node) {
        if (auto next = node->getNextSibling()) {
            return next->getOID();
        }
        node = node->getParent();
    }
    
    return std::vector<uint32_t>();
}

MIBNodePtr MIB::findNode(const std::vector<uint32_t>& oid) const {
    MIBNodePtr current = root_;
    
    for (uint32_t id : oid) {
        current = current->getChild(id);
        if (!current) break;
    }
    
    return current;
}

void MIB::createStandardOIDStructure() {
    // Create standard OID structure: iso(1).org(3).dod(6).internet(1)
    std::vector<std::pair<uint32_t, std::string>> standardPath = {
        {1, "iso"},
        {3, "org"},
        {6, "dod"},
        {1, "internet"},
        {2, "mgmt"},
        {1, "mib-2"}
    };
    
    MIBNodePtr current = root_;
    std::vector<uint32_t> path;
    
    for (const auto& pair : standardPath) {
        path.push_back(pair.first);
        auto node = std::make_shared<MIBNode>(pair.second, pair.first, NodeType::CONTAINER, AccessType::NOT_ACCESSIBLE);
        current->addChild(node);
        current = node;
    }
}

void MIB::createSystemGroup() {
    // System group base: .1.3.6.1.2.1.1
    std::vector<uint32_t> sysBase = {1, 3, 6, 1, 2, 1, 1};
    
    // Add system group nodes
    addNode({1, 3, 6, 1, 2, 1, 1, 1}, "sysDescr", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 2, 1, 1, 2}, "sysObjectID", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 2, 1, 1, 3}, "sysUpTime", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 2, 1, 1, 4}, "sysContact", NodeType::SCALAR, AccessType::READ_WRITE);
    addNode({1, 3, 6, 1, 2, 1, 1, 5}, "sysName", NodeType::SCALAR, AccessType::READ_WRITE);
    addNode({1, 3, 6, 1, 2, 1, 1, 6}, "sysLocation", NodeType::SCALAR, AccessType::READ_WRITE);
}

void MIB::createPrivateGroup() {
    // Private enterprise base: .1.3.6.1.4.1.63050 (using example enterprise number 63050)
    std::vector<uint32_t> privateBase = {1, 3, 6, 1, 4, 1, 63050};
    
    // Add power monitoring nodes
    addNode({1, 3, 6, 1, 4, 1, 63050, 1, 1}, "powerStatus", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 4, 1, 63050, 1, 2}, "lastPowerFailure", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 4, 1, 63050, 1, 3}, "powerFailureCount", NodeType::SCALAR, AccessType::READ_ONLY);
    
    // Add configuration nodes
    addNode({1, 3, 6, 1, 4, 1, 63050, 2, 1}, "ipConfig", NodeType::SCALAR, AccessType::READ_WRITE);
    addNode({1, 3, 6, 1, 4, 1, 63050, 2, 2}, "communityString", NodeType::SCALAR, AccessType::READ_WRITE);
    
    // Add statistics nodes
    addNode({1, 3, 6, 1, 4, 1, 63050, 3, 1}, "totalRequests", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 4, 1, 63050, 3, 2}, "totalResponses", NodeType::SCALAR, AccessType::READ_ONLY);
    addNode({1, 3, 6, 1, 4, 1, 63050, 3, 3}, "errorCount", NodeType::SCALAR, AccessType::READ_ONLY);
}
