#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

#include "MIB.h"
#include "ASN1Types.h"
#include <cstdio>

class SecurityManager {
public:
    explicit SecurityManager(MIB& mib);
    
    // Access control methods
    bool checkAccess(uint32_t clientIP, const char* community);
    void setLogFile(FILE* file);
    
    // Statistics methods
    uint32_t getAccessAttempts() const;
    uint32_t getInvalidAccesses() const;
    uint32_t getRateLimited() const;
    
private:
    static constexpr size_t MAX_CLIENTS = 32;  // Maximum number of tracked clients
    static constexpr unsigned long RATE_LIMIT_WINDOW = 60000;  // 60 seconds
    static constexpr unsigned int MAX_REQUESTS_PER_WINDOW = 100;  // Maximum requests per window
    static constexpr size_t MAX_REASON_LENGTH = 64;  // Maximum length for log reasons
    
    // Client request tracking
    struct ClientRequest {
        unsigned long windowStart;
        unsigned int requestCount;
    };
    
    // MIB OIDs for security statistics
    static constexpr char ACCESS_ATTEMPTS_OID[] = "1.3.6.1.4.1.63050.2.1.0";
    static constexpr char INVALID_ACCESSES_OID[] = "1.3.6.1.4.1.63050.2.2.0";
    static constexpr char RATE_LIMITED_OID[] = "1.3.6.1.4.1.63050.2.3.0";
    
    MIB& mib_;
    FILE* logFile_ = nullptr;
    
    // Client tracking arrays
    uint32_t clientIPs_[MAX_CLIENTS] = {0};
    ClientRequest clientRequests_[MAX_CLIENTS];
    
    // Helper methods
    void logAccess(uint32_t clientIP, bool allowed, const char* reason);
    void incrementCounter(const char* oid);
    uint32_t getCounterValue(const char* oid) const;
    size_t findOrCreateClient(uint32_t clientIP);
    
    // Initialize MIB nodes
    void initializeMIBNodes();
};

#endif // SECURITY_MANAGER_H
