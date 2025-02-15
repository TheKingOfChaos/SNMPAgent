#include "SecurityManager.h"
#include <Arduino.h>
#include <string.h>

SecurityManager::SecurityManager(MIB& mib) : mib_(mib) {
    initializeMIBNodes();
    
    // Initialize client tracking
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        clientRequests_[i] = {0, 0};
        clientIPs_[i] = 0;
    }
}

void SecurityManager::initializeMIBNodes() {
    // Initialize MIB nodes for security statistics
    ASN1Object value(ASN1Object::Type::INTEGER);
    value.setInteger(0);
    
    // Register nodes with initial values
    mib_.registerNode(ACCESS_ATTEMPTS_OID, MIB::NodeType::INTEGER, MIB::Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(0);
            return value;
        });
    
    mib_.registerNode(INVALID_ACCESSES_OID, MIB::NodeType::INTEGER, MIB::Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(0);
            return value;
        });
    
    mib_.registerNode(RATE_LIMITED_OID, MIB::NodeType::INTEGER, MIB::Access::READ_ONLY,
        []() {
            ASN1Object value(ASN1Object::Type::INTEGER);
            value.setInteger(0);
            return value;
        });
}

bool SecurityManager::checkAccess(uint32_t clientIP, const char* community) {
    unsigned long now = millis();
    bool allowed = true;
    
    // Update access attempts counter
    incrementCounter(ACCESS_ATTEMPTS_OID);
    
    // Check community string
    if (strcmp(community, "public") != 0) {  // TODO: Make configurable
        incrementCounter(INVALID_ACCESSES_OID);
        logAccess(clientIP, false, "Invalid community string");
        return false;
    }
    
    // Check rate limiting
    size_t clientIndex = findOrCreateClient(clientIP);
    ClientRequest& client = clientRequests_[clientIndex];
    
    // Reset counter if window has passed
    if (now - client.windowStart > RATE_LIMIT_WINDOW) {
        client.windowStart = now;
        client.requestCount = 0;
    }
    
    // Check if rate limit exceeded
    if (client.requestCount >= MAX_REQUESTS_PER_WINDOW) {
        incrementCounter(RATE_LIMITED_OID);
        logAccess(clientIP, false, "Rate limit exceeded");
        allowed = false;
    } else {
        client.requestCount++;
    }
    
    logAccess(clientIP, allowed, allowed ? "Access granted" : "Rate limited");
    return allowed;
}

void SecurityManager::logAccess(uint32_t clientIP, bool allowed, const char* reason) {
    if (logFile_) {
        unsigned long timestamp = millis();
        char ipStr[16];
        snprintf(ipStr, sizeof(ipStr), "%lu.%lu.%lu.%lu",
                (unsigned long)((clientIP >> 24) & 0xFF),
                (unsigned long)((clientIP >> 16) & 0xFF),
                (unsigned long)((clientIP >> 8) & 0xFF),
                (unsigned long)(clientIP & 0xFF));
        
        // Format: timestamp,ip,allowed,reason
        fprintf(logFile_, "%lu,%s,%d,%s\n",
                timestamp, ipStr, allowed ? 1 : 0, reason);
        fflush(logFile_);
    }
}

void SecurityManager::setLogFile(FILE* file) {
    logFile_ = file;
}

void SecurityManager::incrementCounter(const char* oid) {
    ASN1Object value;
    if (mib_.getValue(oid, value)) {
        int32_t currentValue = value.getInteger();
        value.setInteger(currentValue + 1);
        mib_.setValue(oid, value);
    }
}

uint32_t SecurityManager::getCounterValue(const char* oid) const {
    ASN1Object value;
    if (mib_.getValue(oid, value)) {
        return value.getInteger();
    }
    return 0;
}

size_t SecurityManager::findOrCreateClient(uint32_t clientIP) {
    unsigned long now = millis();
    size_t oldestIndex = 0;
    unsigned long oldestTime = now;
    
    // Look for existing client or oldest entry
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (clientIPs_[i] == clientIP) {
            return i;
        }
        if (clientRequests_[i].windowStart < oldestTime) {
            oldestTime = clientRequests_[i].windowStart;
            oldestIndex = i;
        }
    }
    
    // Use oldest entry for new client
    clientIPs_[oldestIndex] = clientIP;
    clientRequests_[oldestIndex] = {now, 0};
    return oldestIndex;
}

uint32_t SecurityManager::getAccessAttempts() const {
    return getCounterValue(ACCESS_ATTEMPTS_OID);
}

uint32_t SecurityManager::getInvalidAccesses() const {
    return getCounterValue(INVALID_ACCESSES_OID);
}

uint32_t SecurityManager::getRateLimited() const {
    return getCounterValue(RATE_LIMITED_OID);
}
