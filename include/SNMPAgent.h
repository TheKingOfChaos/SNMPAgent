#ifndef SNMP_AGENT_H
#define SNMP_AGENT_H

#include "UDPStack.h"
#include "SecurityManager.h"
#include "MIB.h"
#include "SNMPMessage.h"
#include "ErrorHandler.h"

class SNMPAgent {
public:
    static void processMessages(UDPStack& udp, SecurityManager& security, MIB& mib) {
        uint8_t buffer[1500];
        uint16_t size;
        uint32_t remoteIP;
        uint16_t remotePort;
        
        if (udp.receivePacket(buffer, size, remoteIP, remotePort)) {
            // Check security before processing
            if (security.checkAccess(remoteIP, "public")) {  // TODO: Get community from settings
                SNMPMessage message;
                if (message.decode(buffer, size)) {
                    // Process SNMP request
                    SNMPMessage response;
                    response.createResponse(message, mib);
                    
                    // Send response
                    uint8_t responseBuffer[1500];
                    uint16_t responseSize = response.encode(responseBuffer, sizeof(responseBuffer));
                    if (responseSize > 0) {
                        udp.sendPacket(responseBuffer, responseSize, remoteIP, remotePort);
                    }
                } else {
                    REPORT_ERROR(ErrorHandler::Severity::WARNING,
                               ErrorHandler::Category::PROTOCOL,
                               0x4001,
                               "Failed to decode SNMP message");
                }
            }
        }
    }
};

#endif // SNMP_AGENT_H
