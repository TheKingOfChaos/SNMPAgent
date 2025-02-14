#include "SNMPMessage.h"
#include "MIB.h"
#include <string.h>

namespace SNMP {

// Initialize static request ID counter
int32_t Message::nextRequestID = 1;

// VarBind implementation
VarBind::VarBind(const ASN1::ObjectIdentifier& oid, ASN1::Type* value)
    : oid(oid), value(value) {}

size_t VarBind::encode(uint8_t* buffer, size_t size) const {
    // Create sequence for OID and value
    ASN1::Sequence seq;
    
    // Encode OID
    uint8_t oidBuffer[64];
    size_t oidSize = oid.encode(oidBuffer, sizeof(oidBuffer));
    if (oidSize == 0) return 0;
    if (!seq.addData(oidBuffer, oidSize)) return 0;
    
    // Encode value (or NULL if no value provided)
    if (value) {
        uint8_t valueBuffer[256];
        size_t valueSize = value->encode(valueBuffer, sizeof(valueBuffer));
        if (valueSize == 0) return 0;
        if (!seq.addData(valueBuffer, valueSize)) return 0;
    } else {
        ASN1::Null nullValue;
        uint8_t nullBuffer[2];
        size_t nullSize = nullValue.encode(nullBuffer, sizeof(nullBuffer));
        if (nullSize == 0) return 0;
        if (!seq.addData(nullBuffer, nullSize)) return 0;
    }
    
    // Encode final sequence
    return seq.encode(buffer, size);
}

bool VarBind::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    ASN1::Sequence seq;
    if (!seq.decode(buffer, size, bytesRead)) return false;
    
    // Decode OID
    size_t oidBytes;
    if (!oid.decode(buffer + bytesRead, size - bytesRead, oidBytes)) return false;
    bytesRead += oidBytes;
    
    // Decode value based on type tag
    if (bytesRead >= size) return false;
    
    uint8_t typeTag = buffer[bytesRead];
    size_t valueBytes;
    
    switch (typeTag) {
        case ASN1::INTEGER_TAG:
        case ASN1::COUNTER_TAG:
        case ASN1::GAUGE_TAG:
        case ASN1::TIMETICKS_TAG: {
            auto intValue = new ASN1::Integer();
            if (!intValue->decode(buffer + bytesRead, size - bytesRead, valueBytes)) {
                delete intValue;
                return false;
            }
            value = intValue;
            break;
        }
        case ASN1::OCTET_STRING_TAG: {
            auto strValue = new ASN1::OctetString();
            if (!strValue->decode(buffer + bytesRead, size - bytesRead, valueBytes)) {
                delete strValue;
                return false;
            }
            value = strValue;
            break;
        }
        case ASN1::OBJECT_IDENTIFIER_TAG: {
            auto oidValue = new ASN1::ObjectIdentifier();
            if (!oidValue->decode(buffer + bytesRead, size - bytesRead, valueBytes)) {
                delete oidValue;
                return false;
            }
            value = oidValue;
            break;
        }
        case ASN1::NULL_TAG: {
            auto nullValue = new ASN1::Null();
            if (!nullValue->decode(buffer + bytesRead, size - bytesRead, valueBytes)) {
                delete nullValue;
                return false;
            }
            value = nullValue;
            break;
        }
        default:
            return false;
    }
    
    bytesRead += valueBytes;
    
    return true;
}

// PDU implementation
PDU::PDU(PDUType type)
    : type(type), requestID(0), errorStatus(ErrorStatus::NoError), 
      errorIndex(0), varBindCount(0) {}

size_t PDU::encode(uint8_t* buffer, size_t size) const {
    // Create sequence for PDU contents
    ASN1::Sequence seq;
    
    // Encode request ID
    ASN1::Integer reqID(requestID);
    uint8_t reqIDBuffer[8];
    size_t reqIDSize = reqID.encode(reqIDBuffer, sizeof(reqIDBuffer));
    if (reqIDSize == 0) return 0;
    if (!seq.addData(reqIDBuffer, reqIDSize)) return 0;
    
    // Encode error status
    ASN1::Integer errStatus(static_cast<int32_t>(errorStatus));
    uint8_t errStatusBuffer[8];
    size_t errStatusSize = errStatus.encode(errStatusBuffer, sizeof(errStatusBuffer));
    if (errStatusSize == 0) return 0;
    if (!seq.addData(errStatusBuffer, errStatusSize)) return 0;
    
    // Encode error index
    ASN1::Integer errIdx(errorIndex);
    uint8_t errIdxBuffer[8];
    size_t errIdxSize = errIdx.encode(errIdxBuffer, sizeof(errIdxBuffer));
    if (errIdxSize == 0) return 0;
    if (!seq.addData(errIdxBuffer, errIdxSize)) return 0;
    
    // Create sequence for variable bindings
    ASN1::Sequence varBindSeq;
    for (uint8_t i = 0; i < varBindCount; i++) {
        uint8_t varBindBuffer[512];
        size_t varBindSize = varBinds[i].encode(varBindBuffer, sizeof(varBindBuffer));
        if (varBindSize == 0) return 0;
        if (!varBindSeq.addData(varBindBuffer, varBindSize)) return 0;
    }
    
    // Add varbind sequence to main sequence
    uint8_t varBindSeqBuffer[1024];
    size_t varBindSeqSize = varBindSeq.encode(varBindSeqBuffer, sizeof(varBindSeqBuffer));
    if (varBindSeqSize == 0) return 0;
    if (!seq.addData(varBindSeqBuffer, varBindSeqSize)) return 0;
    
    // Encode final PDU with correct tag
    buffer[0] = static_cast<uint8_t>(type);
    uint8_t seqBuffer[2048];
    size_t seqSize = seq.encode(seqBuffer + 1, sizeof(seqBuffer) - 1);
    if (seqSize == 0) return 0;
    
    memcpy(buffer + 1, seqBuffer + 1, seqSize - 1);
    return seqSize;
}

bool PDU::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    if (size < 2) return false;
    
    // Validate PDU type
    type = static_cast<PDUType>(buffer[0]);
    if (type != PDUType::GetRequest && type != PDUType::GetNextRequest &&
        type != PDUType::GetResponse) {
        return false;
    }
    
    ASN1::Sequence seq;
    if (!seq.decode(buffer, size, bytesRead)) return false;
    
    const uint8_t* curr = buffer + bytesRead;
    size_t remaining = size - bytesRead;
    
    // Decode request ID
    ASN1::Integer reqID;
    size_t reqIDBytes;
    if (!reqID.decode(curr, remaining, reqIDBytes)) return false;
    requestID = reqID.getValue();
    curr += reqIDBytes;
    remaining -= reqIDBytes;
    
    // Decode error status
    ASN1::Integer errStatus;
    size_t errStatusBytes;
    if (!errStatus.decode(curr, remaining, errStatusBytes)) return false;
    errorStatus = static_cast<ErrorStatus>(errStatus.getValue());
    curr += errStatusBytes;
    remaining -= errStatusBytes;
    
    // Decode error index
    ASN1::Integer errIdx;
    size_t errIdxBytes;
    if (!errIdx.decode(curr, remaining, errIdxBytes)) return false;
    errorIndex = errIdx.getValue();
    curr += errIdxBytes;
    remaining -= errIdxBytes;
    
    // Decode variable bindings sequence
    ASN1::Sequence varBindSeq;
    size_t varBindSeqBytes;
    if (!varBindSeq.decode(curr, remaining, varBindSeqBytes)) return false;
    
    // Process each varbind in the sequence
    curr += varBindSeqBytes;
    remaining -= varBindSeqBytes;
    varBindCount = 0;
    
    while (remaining > 0 && varBindCount < 16) {
        VarBind vb;
        size_t vbBytes;
        if (!vb.decode(curr, remaining, vbBytes)) break;
        
        varBinds[varBindCount++] = vb;
        curr += vbBytes;
        remaining -= vbBytes;
    }
    
    bytesRead = size - remaining;
    return true;
}

bool PDU::addVarBind(const VarBind& varBind) {
    if (varBindCount >= 16) return false;
    varBinds[varBindCount++] = varBind;
    return true;
}

void PDU::clear() {
    requestID = 0;
    errorStatus = ErrorStatus::NoError;
    errorIndex = 0;
    varBindCount = 0;
}

// Message implementation
Message::Message() : version(SNMP_VERSION_1) {}

size_t Message::encode(uint8_t* buffer, size_t size) const {
    // Create sequence for entire message
    ASN1::Sequence seq;
    
    // Encode version
    uint8_t versionBuffer[8];
    size_t versionSize = version.encode(versionBuffer, sizeof(versionBuffer));
    if (versionSize == 0) return 0;
    if (!seq.addData(versionBuffer, versionSize)) return 0;
    
    // Encode community string
    uint8_t communityBuffer[256];
    size_t communitySize = community.encode(communityBuffer, sizeof(communityBuffer));
    if (communitySize == 0) return 0;
    if (!seq.addData(communityBuffer, communitySize)) return 0;
    
    // Encode PDU
    uint8_t pduBuffer[2048];
    size_t pduSize = pdu.encode(pduBuffer, sizeof(pduBuffer));
    if (pduSize == 0) return 0;
    if (!seq.addData(pduBuffer, pduSize)) return 0;
    
    // Encode final message sequence
    return seq.encode(buffer, size);
}

bool Message::decode(const uint8_t* buffer, size_t size) {
    ASN1::Sequence seq;
    size_t bytesRead;
    if (!seq.decode(buffer, size, bytesRead)) return false;
    
    const uint8_t* curr = buffer + bytesRead;
    size_t remaining = size - bytesRead;
    
    // Decode version
    size_t versionBytes;
    if (!version.decode(curr, remaining, versionBytes)) return false;
    if (version.getValue() != SNMP_VERSION_1) return false;
    curr += versionBytes;
    remaining -= versionBytes;
    
    // Decode community string
    size_t communityBytes;
    if (!community.decode(curr, remaining, communityBytes)) return false;
    curr += communityBytes;
    remaining -= communityBytes;
    
    // Decode PDU
    size_t pduBytes;
    if (!pdu.decode(curr, remaining, pduBytes)) return false;
    
    return true;
}

bool Message::validateCommunity(const char* expectedCommunity) const {
    return strcmp(community.getValue(), expectedCommunity) == 0;
}

Message Message::createGetRequest(const char* community, const ASN1::ObjectIdentifier& oid) {
    Message msg;
    msg.community.setValue(community);
    msg.pdu.type = PDUType::GetRequest;
    msg.pdu.requestID = nextRequestID++;
    
    VarBind vb(oid);
    msg.pdu.addVarBind(vb);
    
    return msg;
}

Message Message::createGetNextRequest(const char* community, const ASN1::ObjectIdentifier& oid) {
    Message msg;
    msg.community.setValue(community);
    msg.pdu.type = PDUType::GetNextRequest;
    msg.pdu.requestID = nextRequestID++;
    
    VarBind vb(oid);
    msg.pdu.addVarBind(vb);
    
    return msg;
}

Message Message::createGetResponse(const Message& request, const VarBind& response) {
    Message msg;
    msg.community = request.community;
    msg.pdu.type = PDUType::GetResponse;
    msg.pdu.requestID = request.pdu.requestID;
    msg.pdu.addVarBind(response);
    
    return msg;
}

Message Message::createErrorResponse(const Message& request, ErrorStatus status, int32_t index) {
    Message msg;
    msg.community = request.community;
    msg.pdu.type = PDUType::GetResponse;
    msg.pdu.requestID = request.pdu.requestID;
    msg.pdu.errorStatus = status;
    msg.pdu.errorIndex = index;
    
    // Copy the original varbinds but with NULL values
    for (uint8_t i = 0; i < request.pdu.varBindCount; i++) {
        VarBind vb(request.pdu.varBinds[i].oid);
        msg.pdu.addVarBind(vb);
    }
    
    return msg;
}

// Initialize static rate limiting members
uint32_t Message::requestTimes[MAX_REQUESTS_PER_WINDOW] = {0};
uint8_t Message::requestTimeIndex = 0;

bool Message::checkRateLimit() {
    uint32_t currentTime = millis();
    uint32_t windowStart = currentTime - RATE_LIMIT_WINDOW_MS;
    
    // Count requests within the window
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_REQUESTS_PER_WINDOW; i++) {
        if (requestTimes[i] > windowStart) {
            count++;
        }
    }
    
    return count < MAX_REQUESTS_PER_WINDOW;
}

void Message::updateRateLimit() {
    requestTimes[requestTimeIndex] = millis();
    requestTimeIndex = (requestTimeIndex + 1) % MAX_REQUESTS_PER_WINDOW;
}

} // namespace SNMP
