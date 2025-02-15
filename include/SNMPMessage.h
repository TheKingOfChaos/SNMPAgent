#ifndef SNMP_MESSAGE_H
#define SNMP_MESSAGE_H

#include "ASN1Object.h"
#include "MIB.h"
#include <cstddef>
#include <cstdint>

class SNMPMessage {
public:
    enum class PDUType {
        GET_REQUEST = 0xA0,
        GET_NEXT_REQUEST = 0xA1,
        GET_RESPONSE = 0xA2,
        SET_REQUEST = 0xA3,
        TRAP = 0xA4
    };
    
    static constexpr size_t MAX_COMMUNITY_LENGTH = 32;
    static constexpr size_t MAX_VARBINDS = 16;
    static constexpr size_t MAX_OID_STRING_LENGTH = 64;
    
    // Convert between string and numeric OID representations
    static bool numericToStringOID(const uint32_t* numericOID, size_t length, char* stringOID, size_t maxLength);
    static bool stringToNumericOID(const char* stringOID, uint32_t* numericOID, size_t* length, size_t maxLength);
    
    struct VarBind {
        char oid[MAX_OID_STRING_LENGTH];
        ASN1Object value;
    };
    
    SNMPMessage();
    
    // Encoding/Decoding
    bool decode(const uint8_t* buffer, uint16_t size);
    uint16_t encode(uint8_t* buffer, uint16_t maxSize);
    
    // Response creation
    void createResponse(const SNMPMessage& request, MIB& mib);
    
    // Getters
    uint8_t getVersion() const { return version_; }
    const char* getCommunity() const { return community_; }
    PDUType getPDUType() const { return pduType_; }
    uint32_t getRequestID() const { return requestID_; }
    uint32_t getErrorStatus() const { return errorStatus_; }
    uint32_t getErrorIndex() const { return errorIndex_; }
    const VarBind* getVarBinds() const { return varBinds_; }
    size_t getVarBindCount() const { return varBind_count_; }
    
    // Setters
    void setVersion(uint8_t version) { version_ = version; }
    void setCommunity(const char* community);
    void setPDUType(PDUType type) { pduType_ = type; }
    void setRequestID(uint32_t id) { requestID_ = id; }
    void setErrorStatus(uint32_t status) { errorStatus_ = status; }
    void setErrorIndex(uint32_t index) { errorIndex_ = index; }
    bool addVarBind(const char* oid, const ASN1Object& value);
    
private:
    uint8_t version_;
    char community_[MAX_COMMUNITY_LENGTH];
    PDUType pduType_;
    uint32_t requestID_;
    uint32_t errorStatus_;
    uint32_t errorIndex_;
    VarBind varBinds_[MAX_VARBINDS];
    size_t varBind_count_;
    
    // Helper methods
    bool decodePDU(const uint8_t* buffer, uint16_t size, uint16_t& offset);
    bool decodeVarBinds(const uint8_t* buffer, uint16_t size, uint16_t& offset);
    uint16_t encodePDU(uint8_t* buffer, uint16_t maxSize);
    uint16_t encodeVarBinds(uint8_t* buffer, uint16_t maxSize);
    
    // Response processing helpers
    void processGetRequest(const SNMPMessage& request, MIB& mib);
    void processGetNextRequest(const SNMPMessage& request, MIB& mib);
};

#endif // SNMP_MESSAGE_H
