#include "SNMPMessage.h"
#include "ASN1Object.h"
#include "ErrorHandler.h"
#include <string.h>
#include <stdio.h>

bool SNMPMessage::numericToStringOID(const uint32_t* numericOID, size_t length, char* stringOID, size_t maxLength) {
    if (!numericOID || !stringOID || !length || !maxLength) {
        return false;
    }
    
    size_t offset = 0;
    offset += snprintf(stringOID + offset, maxLength - offset, "%lu", (unsigned long)numericOID[0]);
    
    for (size_t i = 1; i < length && offset < maxLength; i++) {
        offset += snprintf(stringOID + offset, maxLength - offset, ".%lu", (unsigned long)numericOID[i]);
    }
    
    return offset < maxLength;
}

bool SNMPMessage::stringToNumericOID(const char* stringOID, uint32_t* numericOID, size_t* length, size_t maxLength) {
    if (!stringOID || !numericOID || !length || !maxLength) {
        return false;
    }
    
    *length = 0;
    const char* ptr = stringOID;
    
    while (*ptr && *length < maxLength) {
        // Skip dots
        if (*ptr == '.') {
            ptr++;
            continue;
        }
        
        // Parse number
        char* end;
        unsigned long num = strtoul(ptr, &end, 10);
        if (end == ptr) {
            return false;
        }
        
        numericOID[(*length)++] = num;
        ptr = end;
    }
    
    return true;
}

SNMPMessage::SNMPMessage()
    : version_(0)
    , pduType_(PDUType::GET_REQUEST)
    , requestID_(0)
    , errorStatus_(0)
    , errorIndex_(0)
    , varBind_count_(0)
{
    community_[0] = '\0';
}

void SNMPMessage::setCommunity(const char* community) {
    strncpy(community_, community, MAX_COMMUNITY_LENGTH - 1);
    community_[MAX_COMMUNITY_LENGTH - 1] = '\0';
}

bool SNMPMessage::addVarBind(const char* oid, const ASN1Object& value) {
    if (varBind_count_ >= MAX_VARBINDS) {
        return false;
    }
    
    strncpy(varBinds_[varBind_count_].oid, oid, MAX_OID_STRING_LENGTH - 1);
    varBinds_[varBind_count_].oid[MAX_OID_STRING_LENGTH - 1] = '\0';
    varBinds_[varBind_count_].value = value;
    varBind_count_++;
    
    return true;
}

bool SNMPMessage::decode(const uint8_t* buffer, uint16_t size) {
    if (!buffer || size < 2) {
        REPORT_ERROR(ErrorHandler::Severity::WARNING,
                    ErrorHandler::Category::PROTOCOL,
                    0x4002,
                    "Invalid SNMP message buffer");
        return false;
    }
    
    uint16_t offset = 0;
    varBind_count_ = 0;
    
    // Decode SNMP message sequence
    ASN1Object sequence;
    if (!sequence.decode(buffer, size, offset)) {
        REPORT_ERROR(ErrorHandler::Severity::WARNING,
                    ErrorHandler::Category::PROTOCOL,
                    0x4003,
                    "Failed to decode SNMP sequence");
        return false;
    }
    
    // Decode version
    ASN1Object versionObj;
    if (!versionObj.decode(buffer, size, offset)) {
        REPORT_ERROR(ErrorHandler::Severity::WARNING,
                    ErrorHandler::Category::PROTOCOL,
                    0x4004,
                    "Failed to decode SNMP version");
        return false;
    }
    version_ = versionObj.getInteger();
    
    // Decode community string
    ASN1Object communityObj;
    if (!communityObj.decode(buffer, size, offset)) {
        REPORT_ERROR(ErrorHandler::Severity::WARNING,
                    ErrorHandler::Category::PROTOCOL,
                    0x4005,
                    "Failed to decode community string");
        return false;
    }
    setCommunity(communityObj.getString());
    
    // Decode PDU
    return decodePDU(buffer, size, offset);
}

bool SNMPMessage::decodePDU(const uint8_t* buffer, uint16_t size, uint16_t& offset) {
    // Get PDU type
    if (offset >= size) {
        return false;
    }
    pduType_ = static_cast<PDUType>(buffer[offset]);
    
    // Decode PDU sequence
    ASN1Object pduSequence;
    if (!pduSequence.decode(buffer, size, offset)) {
        return false;
    }
    
    // Decode request ID
    ASN1Object requestIDObj;
    if (!requestIDObj.decode(buffer, size, offset)) {
        return false;
    }
    requestID_ = requestIDObj.getInteger();
    
    // Decode error status
    ASN1Object errorStatusObj;
    if (!errorStatusObj.decode(buffer, size, offset)) {
        return false;
    }
    errorStatus_ = errorStatusObj.getInteger();
    
    // Decode error index
    ASN1Object errorIndexObj;
    if (!errorIndexObj.decode(buffer, size, offset)) {
        return false;
    }
    errorIndex_ = errorIndexObj.getInteger();
    
    // Decode variable bindings
    return decodeVarBinds(buffer, size, offset);
}

bool SNMPMessage::decodeVarBinds(const uint8_t* buffer, uint16_t size, uint16_t& offset) {
    // Decode varbind sequence
    ASN1Object varbindSequence;
    if (!varbindSequence.decode(buffer, size, offset)) {
        return false;
    }
    
    // Reset varbind count
    varBind_count_ = 0;
    
    // Decode each varbind
    while (offset < size && varBind_count_ < MAX_VARBINDS) {
        // Decode varbind sequence
        ASN1Object varbindObj;
        if (!varbindObj.decode(buffer, size, offset)) {
            break;
        }
        
        // Decode OID
        ASN1Object oidObj;
        if (!oidObj.decode(buffer, size, offset)) {
            return false;
        }
        
        // Decode value
        ASN1Object valueObj;
        if (!valueObj.decode(buffer, size, offset)) {
            return false;
        }
        
        // Convert numeric OID to string and add to varbind list
        if (!numericToStringOID(oidObj.getOID(), oidObj.getOIDLength(),
                              varBinds_[varBind_count_].oid, MAX_OID_STRING_LENGTH)) {
            return false;
        }
        
        varBinds_[varBind_count_].value = valueObj;
        varBind_count_++;
    }
    
    return true;
}

uint16_t SNMPMessage::encode(uint8_t* buffer, uint16_t maxSize) {
    if (!buffer || maxSize < 2) {
        return 0;
    }
    
    uint16_t offset = 0;
    
    // Start SNMP sequence
    buffer[offset++] = 0x30; // Sequence tag
    uint16_t lengthOffset = offset++;
    
    // Encode version
    ASN1Object versionObj(ASN1Object::Type::INTEGER);
    versionObj.setInteger(version_);
    offset += versionObj.encode(buffer + offset, maxSize - offset);
    
    // Encode community string
    ASN1Object communityObj(ASN1Object::Type::OCTET_STRING);
    communityObj.setString(community_, strlen(community_));
    offset += communityObj.encode(buffer + offset, maxSize - offset);
    
    // Encode PDU
    offset += encodePDU(buffer + offset, maxSize - offset);
    
    // Update sequence length
    buffer[lengthOffset] = offset - 2;
    
    return offset;
}

uint16_t SNMPMessage::encodePDU(uint8_t* buffer, uint16_t maxSize) {
    if (!buffer || maxSize < 2) {
        return 0;
    }
    
    uint16_t offset = 0;
    
    // Write PDU type
    buffer[offset++] = static_cast<uint8_t>(pduType_);
    uint16_t lengthOffset = offset++;
    
    // Encode request ID
    ASN1Object requestIDObj(ASN1Object::Type::INTEGER);
    requestIDObj.setInteger(requestID_);
    offset += requestIDObj.encode(buffer + offset, maxSize - offset);
    
    // Encode error status
    ASN1Object errorStatusObj(ASN1Object::Type::INTEGER);
    errorStatusObj.setInteger(errorStatus_);
    offset += errorStatusObj.encode(buffer + offset, maxSize - offset);
    
    // Encode error index
    ASN1Object errorIndexObj(ASN1Object::Type::INTEGER);
    errorIndexObj.setInteger(errorIndex_);
    offset += errorIndexObj.encode(buffer + offset, maxSize - offset);
    
    // Encode variable bindings
    offset += encodeVarBinds(buffer + offset, maxSize - offset);
    
    // Update PDU length
    buffer[lengthOffset] = offset - 2;
    
    return offset;
}

uint16_t SNMPMessage::encodeVarBinds(uint8_t* buffer, uint16_t maxSize) {
    if (!buffer || maxSize < 2) {
        return 0;
    }
    
    uint16_t offset = 0;
    
    // Start varbind sequence
    buffer[offset++] = 0x30; // Sequence tag
    uint16_t lengthOffset = offset++;
    
    // Encode each varbind
    for (size_t i = 0; i < varBind_count_; i++) {
        // Start varbind sequence
        buffer[offset++] = 0x30;
        uint16_t varbindLengthOffset = offset++;
        
        // Convert string OID to numeric and encode
        uint32_t numericOID[ASN1Object::MAX_OID_LENGTH];
        size_t oidLength;
        if (!stringToNumericOID(varBinds_[i].oid, numericOID, &oidLength, ASN1Object::MAX_OID_LENGTH)) {
            return 0;
        }
        
        ASN1Object oidObj(ASN1Object::Type::OBJECT_IDENTIFIER);
        oidObj.setOID(numericOID, oidLength);
        offset += oidObj.encode(buffer + offset, maxSize - offset);
        
        // Encode value
        offset += varBinds_[i].value.encode(buffer + offset, maxSize - offset);
        
        // Update varbind length
        buffer[varbindLengthOffset] = offset - varbindLengthOffset - 1;
    }
    
    // Update sequence length
    buffer[lengthOffset] = offset - 2;
    
    return offset;
}

// createResponse implementation moved to SNMPMessageMIB.cpp
