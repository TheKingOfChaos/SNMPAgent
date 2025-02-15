#include "SNMPMessage.h"
#include "MIB.h"
#include "ASN1Object.h"
#include <cstddef>

void SNMPMessage::createResponse(const SNMPMessage& request, MIB& mib) {
    // Copy request fields
    setVersion(request.getVersion());
    setCommunity(request.getCommunity());
    setRequestID(request.getRequestID());
    
    // Set response type
    setPDUType(PDUType::GET_RESPONSE);
    
    // Clear error status
    setErrorStatus(0);
    setErrorIndex(0);
    
    // Process based on request PDU type
    switch (request.getPDUType()) {
        case PDUType::GET_REQUEST:
            this->processGetRequest(request, mib);
            break;
        case PDUType::GET_NEXT_REQUEST:
            this->processGetNextRequest(request, mib);
            break;
        default:
            setErrorStatus(5); // genErr
            break;
    }
}

void SNMPMessage::processGetRequest(const SNMPMessage& request, MIB& mib) {
    // Process each varbind
    const VarBind* requestVarBinds = request.getVarBinds();
    size_t varBindCount = request.getVarBindCount();
    
    for (size_t i = 0; i < varBindCount; i++) {
        const VarBind& requestVarBind = requestVarBinds[i];
        ASN1Object value;
        
        // Get value for OID
        if (!mib.getValue(requestVarBind.oid, value)) {
            // OID not found
            setErrorStatus(2); // noSuchName
            setErrorIndex(i + 1);
            return;
        }
        
        // Add response varbind
        if (!addVarBind(requestVarBind.oid, value)) {
            // Too many varbinds
            setErrorStatus(1); // tooBig
            setErrorIndex(0);
            return;
        }
    }
}

void SNMPMessage::processGetNextRequest(const SNMPMessage& request, MIB& mib) {
    // Process each varbind
    const VarBind* requestVarBinds = request.getVarBinds();
    size_t varBindCount = request.getVarBindCount();
    
    for (size_t i = 0; i < varBindCount; i++) {
        const VarBind& requestVarBind = requestVarBinds[i];
        char nextOid[MAX_OID_STRING_LENGTH];
        ASN1Object value;
        
        // Get next OID
        if (!mib.getNextOID(requestVarBind.oid, nextOid, sizeof(nextOid))) {
            // No next OID available
            setErrorStatus(2); // noSuchName
            setErrorIndex(i + 1);
            return;
        }
        
        // Get value for next OID
        if (!mib.getValue(nextOid, value)) {
            setErrorStatus(5); // genErr
            setErrorIndex(i + 1);
            return;
        }
        
        // Add response varbind
        if (!addVarBind(nextOid, value)) {
            // Too many varbinds
            setErrorStatus(1); // tooBig
            setErrorIndex(0);
            return;
        }
    }
}
