#include "SNMPMessage.h"
#include "MIB.h"
#include "ASN1Object.h"
#include <vector>

namespace SNMP {

bool Message::processRequest(const MIB& mib, Message& response) {
    // Check rate limit
    if (!checkRateLimit()) {
        response = createErrorResponse(*this, ErrorStatus::TooBig);
        return true;
    }
    updateRateLimit();
    
    // Process based on PDU type
    switch (pdu.type) {
        case PDUType::GetRequest:
            return processGetRequest(mib, response);
        case PDUType::GetNextRequest:
            return processGetNextRequest(mib, response);
        default:
            response = createErrorResponse(*this, ErrorStatus::GenError);
            return true;
    }
}

bool Message::processGetRequest(const MIB& mib, Message& response) {
    response = Message();
    response.community = community;
    response.pdu.type = PDUType::GetResponse;
    response.pdu.requestID = pdu.requestID;
    
    // Process each varbind
    for (uint8_t i = 0; i < pdu.varBindCount; i++) {
        const VarBind& request = pdu.varBinds[i];
        
        // Convert ASN1::ObjectIdentifier to vector<uint32_t>
        std::vector<uint32_t> oid(request.oid.getComponents(), 
                                 request.oid.getComponents() + request.oid.getComponentCount());
        
        ASN1::Type* value = static_cast<ASN1::Type*>(mib.getValue(oid));
        
        if (!value) {
            // OID not found
            response = createErrorResponse(*this, ErrorStatus::NoSuchName, i + 1);
            return true;
        }
        
        VarBind response_vb(request.oid, value);
        response.pdu.addVarBind(response_vb);
    }
    
    return true;
}

bool Message::processGetNextRequest(const MIB& mib, Message& response) {
    response = Message();
    response.community = community;
    response.pdu.type = PDUType::GetResponse;
    response.pdu.requestID = pdu.requestID;
    
    // Process each varbind
    for (uint8_t i = 0; i < pdu.varBindCount; i++) {
        const VarBind& request = pdu.varBinds[i];
        
        // Convert ASN1::ObjectIdentifier to vector<uint32_t>
        std::vector<uint32_t> oid(request.oid.getComponents(), 
                                 request.oid.getComponents() + request.oid.getComponentCount());
        
        // Get next OID
        std::vector<uint32_t> nextOid = mib.getNextOID(oid);
        if (nextOid.empty()) {
            // No next OID available
            response = createErrorResponse(*this, ErrorStatus::NoSuchName, i + 1);
            return true;
        }
        
        // Get value for next OID
        ASN1::Type* value = static_cast<ASN1::Type*>(mib.getValue(nextOid));
        if (!value) {
            response = createErrorResponse(*this, ErrorStatus::GenError, i + 1);
            return true;
        }
        
        // Convert vector<uint32_t> back to ASN1::ObjectIdentifier
        ASN1::ObjectIdentifier nextOidObj;
        nextOidObj.setComponents(nextOid.data(), nextOid.size());
        
        VarBind response_vb(nextOidObj, value);
        response.pdu.addVarBind(response_vb);
    }
    
    return true;
}

} // namespace SNMP
