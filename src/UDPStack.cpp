#include "UDPStack.h"
#include <Arduino.h>

// DHCP Message Types
#define DHCP_DISCOVER    1
#define DHCP_OFFER      2
#define DHCP_REQUEST    3
#define DHCP_ACK        5

// DHCP Configuration
#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68
#define DHCP_TIMEOUT        10000  // 10 seconds

UDPStack::UDPStack(W5500& eth)
    : eth_(eth)
    , dhcpEnabled_(false)
    , lastDHCPRenewal_(0) {
}

bool UDPStack::openSocket(uint8_t socket, uint16_t port) {
    // Use W5500's UDP packet handling methods
    return eth_.beginPacket(nullptr, port);
}

void UDPStack::closeSocket(uint8_t socket) {
    // End any ongoing packet operation
    eth_.endPacket();
}

bool UDPStack::receivePacket(uint8_t* buffer, uint16_t& size, uint32_t& remoteIP, uint16_t& remotePort) {
    if (!waitForData(0, 1000)) {  // Use socket 0, 1 second timeout
        return false;
    }
    
    int packetSize = eth_.parsePacket();
    if (packetSize <= 0) {
        return false;
    }
    
    size = eth_.read(buffer, packetSize);
    return size > 0;
}

bool UDPStack::sendPacket(const uint8_t* buffer, uint16_t size, uint32_t remoteIP, uint16_t remotePort) {
    if (!eth_.beginPacket(reinterpret_cast<const uint8_t*>(&remoteIP), remotePort)) {
        return false;
    }
    
    if (eth_.write(buffer, size) != size) {
        return false;
    }
    
    return eth_.endPacket();
}

bool UDPStack::startDHCP() {
    if (dhcpEnabled_) {
        return true;
    }
    
    if (!eth_.startDHCP()) {
        return false;
    }
    
    dhcpEnabled_ = true;
    lastDHCPRenewal_ = millis();
    return true;
}

void UDPStack::stopDHCP() {
    if (dhcpEnabled_) {
        eth_.stopDHCP();
        dhcpEnabled_ = false;
    }
}

void UDPStack::renewDHCP() {
    if (!dhcpEnabled_) {
        return;
    }
    
    if (millis() - lastDHCPRenewal_ >= DHCP_RENEWAL_INTERVAL) {
        if (eth_.renewDHCP()) {
            lastDHCPRenewal_ = millis();
        }
    }
}

bool UDPStack::isConnected() const {
    return eth_.isLinked();
}

bool UDPStack::sendDHCPDiscover() {
    // TODO: Implement DHCP discover message using eth_.write()
    return true;
}

bool UDPStack::receiveDHCPOffer() {
    // TODO: Implement DHCP offer reception using eth_.read()
    return true;
}

bool UDPStack::sendDHCPRequest() {
    // TODO: Implement DHCP request message using eth_.write()
    return true;
}

bool UDPStack::receiveDHCPAck() {
    // TODO: Implement DHCP ACK reception using eth_.read()
    return true;
}

bool UDPStack::waitForData(uint8_t socket, uint32_t timeout) {
    uint32_t start = millis();
    while (millis() - start < timeout) {
        if (eth_.parsePacket() > 0) {
            return true;
        }
        delay(1);
    }
    return false;
}

uint16_t UDPStack::getAvailableData(uint8_t socket) {
    return eth_.parsePacket();
}
