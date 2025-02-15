#ifndef UDP_STACK_H
#define UDP_STACK_H

#include "W5500.h"
#include <cstdint>

class UDPStack {
public:
    explicit UDPStack(W5500& eth);
    
    // DHCP methods
    bool startDHCP();
    void stopDHCP();
    void renewDHCP();
    bool isConnected() const;
    
    // UDP packet handling
    bool receivePacket(uint8_t* buffer, uint16_t& size, uint32_t& remoteIP, uint16_t& remotePort);
    bool sendPacket(const uint8_t* buffer, uint16_t size, uint32_t remoteIP, uint16_t remotePort);
    
    // Socket management
    bool openSocket(uint8_t socket, uint16_t port);
    void closeSocket(uint8_t socket);
    
private:
    W5500& eth_;
    bool dhcpEnabled_;
    uint32_t lastDHCPRenewal_;
    static constexpr uint32_t DHCP_RENEWAL_INTERVAL = 300000;  // 5 minutes
    
    // DHCP helper methods
    bool sendDHCPDiscover();
    bool receiveDHCPOffer();
    bool sendDHCPRequest();
    bool receiveDHCPAck();
    
    // UDP helper methods
    bool waitForData(uint8_t socket, uint32_t timeout);
    uint16_t getAvailableData(uint8_t socket);
};

#endif // UDP_STACK_H
