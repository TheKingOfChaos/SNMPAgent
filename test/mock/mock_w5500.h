#ifndef MOCK_W5500_H
#define MOCK_W5500_H

#include <vector>
#include <queue>
#include <cstring>
#include <cstdint>
#include <Arduino.h>

// Mock W5500 network interface for testing
class MockW5500 {
public:
    // Network configuration
    struct NetworkConfig {
        uint8_t mac[6];
        uint32_t ip;
        uint32_t subnet;
        uint32_t gateway;
        bool dhcp;
    };
    
    // Socket status
    enum SocketStatus {
        CLOSED,
        UDP,
        ERROR
    };
    
    // Packet structure
    struct Packet {
        uint8_t data[1024];
        size_t length;
        uint32_t sourceIP;
        uint16_t sourcePort;
    };
    
    MockW5500() {
        // Default configuration
        memset(&config, 0, sizeof(config));
        config.dhcp = true;
        
        // Initialize socket status
        for (int i = 0; i < 8; i++) {
            socketStatus[i] = CLOSED;
        }
    }
    
    // Network configuration methods
    bool begin(const uint8_t* mac) {
        memcpy(config.mac, mac, 6);
        return true;
    }
    
    bool setDHCP(bool enabled) {
        config.dhcp = enabled;
        return true;
    }
    
    bool setStaticIP(uint32_t ip, uint32_t subnet, uint32_t gateway) {
        if (!config.dhcp) {
            config.ip = ip;
            config.subnet = subnet;
            config.gateway = gateway;
            return true;
        }
        return false;
    }
    
    // Socket operations
    bool openSocket(uint8_t socket, uint16_t port) {
        if (socket < 8 && socketStatus[socket] == CLOSED) {
            socketStatus[socket] = UDP;
            socketPorts[socket] = port;
            return true;
        }
        return false;
    }
    
    bool closeSocket(uint8_t socket) {
        if (socket < 8) {
            socketStatus[socket] = CLOSED;
            return true;
        }
        return false;
    }
    
    // Packet handling
    bool sendPacket(uint8_t socket, const uint8_t* data, size_t length, uint32_t destIP, uint16_t destPort) {
        if (socket < 8 && socketStatus[socket] == UDP && length <= 1024) {
            Packet packet;
            memcpy(packet.data, data, length);
            packet.length = length;
            packet.sourceIP = config.ip;
            packet.sourcePort = socketPorts[socket];
            sentPackets.push(packet);
            return true;
        }
        return false;
    }
    
    bool receivePacket(uint8_t socket, uint8_t* buffer, size_t& length, uint32_t& sourceIP, uint16_t& sourcePort) {
        if (socket < 8 && socketStatus[socket] == UDP && !receivedPackets.empty()) {
            Packet packet = receivedPackets.front();
            receivedPackets.pop();
            
            if (length >= packet.length) {
                memcpy(buffer, packet.data, packet.length);
                length = packet.length;
                sourceIP = packet.sourceIP;
                sourcePort = packet.sourcePort;
                return true;
            }
        }
        return false;
    }
    
    // Test helper methods
    void queueReceivedPacket(const uint8_t* data, size_t length, uint32_t sourceIP, uint16_t sourcePort) {
        if (length <= 1024) {
            Packet packet;
            memcpy(packet.data, data, length);
            packet.length = length;
            packet.sourceIP = sourceIP;
            packet.sourcePort = sourcePort;
            receivedPackets.push(packet);
        }
    }
    
    bool getLastSentPacket(uint8_t* buffer, size_t& length, uint32_t& sourceIP, uint16_t& sourcePort) {
        if (!sentPackets.empty()) {
            Packet packet = sentPackets.front();
            sentPackets.pop();
            
            memcpy(buffer, packet.data, packet.length);
            length = packet.length;
            sourceIP = packet.sourceIP;
            sourcePort = packet.sourcePort;
            return true;
        }
        return false;
    }
    
    // Status methods
    SocketStatus getSocketStatus(uint8_t socket) {
        return socket < 8 ? socketStatus[socket] : ERROR;
    }
    
    NetworkConfig getNetworkConfig() const {
        return config;
    }
    
private:
    NetworkConfig config;
    SocketStatus socketStatus[8];
    uint16_t socketPorts[8];
    std::queue<Packet> receivedPackets;
    std::queue<Packet> sentPackets;
};

#endif // MOCK_W5500_H
