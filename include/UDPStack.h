#ifndef UDP_STACK_H
#define UDP_STACK_H

#include <Arduino.h>
#include "W5500.h"

// UDP Socket States
#define UDP_STATE_CLOSED      0
#define UDP_STATE_OPEN        1

// Maximum UDP packet size
#define UDP_MAX_PACKET_SIZE   1472

class UDPStack {
public:
    UDPStack(W5500& w5500);
    
    // Socket management
    bool begin(uint16_t localPort);
    void close();
    
    // UDP operations
    bool beginPacket(const uint8_t* ip, uint16_t port);
    bool endPacket();
    size_t write(const uint8_t* buffer, size_t size);
    int parsePacket();
    int read(uint8_t* buffer, size_t size);
    int available();
    
    // DHCP client
    bool startDHCP();
    bool renewDHCP();
    void stopDHCP();
    
    // Network configuration
    void setStaticIP(const uint8_t* ip, const uint8_t* gateway, const uint8_t* subnet);
    bool isConnected();
    
private:
    W5500& _w5500;
    uint8_t _socket;
    uint16_t _localPort;
    uint8_t _state;
    uint8_t _remoteIP[4];
    uint16_t _remotePort;
    
    // DHCP state
    bool _dhcp_enabled;
    uint32_t _dhcp_lease_time;
    uint32_t _last_renewal;
    
    // Internal buffer for received data
    uint8_t _rxBuffer[UDP_MAX_PACKET_SIZE];
    size_t _rxSize;
    size_t _rxIndex;
    
    // Helper functions
    bool allocateSocket();
    void releaseSocket();
    bool sendDHCPDiscover();
    bool processDHCPOffer();
    bool sendDHCPRequest();
    bool processDHCPAck();
    void updateDHCPState();
};

#endif // UDP_STACK_H
