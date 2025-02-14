#ifndef SNMP_MESSAGE_H
#define SNMP_MESSAGE_H

#include "ASN1Types.h"
#include <Arduino.h>

// Forward declare global MIB class
class MIB;

namespace SNMP {

// SNMP Version (v1 only as per requirements)
constexpr uint32_t SNMP_VERSION_1 = 0;

// PDU Types (as per RFC 1157)
enum class PDUType : uint8_t {
    GetRequest      = 0xA0,
    GetNextRequest  = 0xA1,
    GetResponse     = 0xA2,
    SetRequest      = 0xA3,  // Not implemented as per requirements
    Trap           = 0xA4   // Not implemented as per requirements
};

// Error Status Codes
enum class ErrorStatus : int32_t {
    NoError          = 0,
    TooBig           = 1,
    NoSuchName       = 2,
    BadValue         = 3,
    ReadOnly         = 4,
    GenError         = 5
};

// Variable Binding structure
class VarBind {
public:
    VarBind() = default;
    VarBind(const ASN1::ObjectIdentifier& oid, ASN1::Type* value = nullptr);
    
    size_t encode(uint8_t* buffer, size_t size) const;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead);
    
    ASN1::ObjectIdentifier oid;
    ASN1::Type* value;  // Can be Integer, OctetString, Null, ObjectIdentifier
};

// PDU structure (common for all PDU types)
class PDU {
public:
    PDU(PDUType type = PDUType::GetRequest);
    
    size_t encode(uint8_t* buffer, size_t size) const;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead);
    
    PDUType type;
    int32_t requestID;
    ErrorStatus errorStatus;
    int32_t errorIndex;
    VarBind varBinds[16];  // Maximum 16 variable bindings per request
    uint8_t varBindCount;
    
    bool addVarBind(const VarBind& varBind);
    void clear();
};

// SNMP Message structure
class Message {
public:
    Message();
    
    // Encode entire SNMP message
    size_t encode(uint8_t* buffer, size_t size) const;
    
    // Decode received SNMP message
    bool decode(const uint8_t* buffer, size_t size);
    
    // Validate community string
    bool validateCommunity(const char* expectedCommunity) const;
    
    // Helper methods for creating specific message types
    static Message createGetRequest(const char* community, const ASN1::ObjectIdentifier& oid);
    static Message createGetNextRequest(const char* community, const ASN1::ObjectIdentifier& oid);
    static Message createGetResponse(const Message& request, const VarBind& response);
    static Message createErrorResponse(const Message& request, ErrorStatus status, int32_t index = 0);
    
    // Request processing (implemented in SNMPMessageMIB.cpp)
    bool processRequest(const ::MIB& mib, Message& response);
    
private:
    bool processGetRequest(const ::MIB& mib, Message& response);
    bool processGetNextRequest(const ::MIB& mib, Message& response);
    
    // Rate limiting
    bool checkRateLimit();
    void updateRateLimit();
    
    // Message components
    ASN1::Integer version;         // SNMP version (always 0 for v1)
    ASN1::OctetString community;   // Community string
    PDU pdu;                      // Protocol Data Unit
    
    // Rate limiting configuration
    static constexpr uint32_t RATE_LIMIT_WINDOW_MS = 1000;  // 1 second window
    static constexpr uint8_t MAX_REQUESTS_PER_WINDOW = 10;  // Max 10 requests per second
    static uint32_t requestTimes[MAX_REQUESTS_PER_WINDOW];
    static uint8_t requestTimeIndex;
    
    // Request ID counter
    static int32_t nextRequestID;  // Counter for generating request IDs
};

} // namespace SNMP

#endif // SNMP_MESSAGE_H
