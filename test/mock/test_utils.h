#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "mock_w5500.h"
#include "mock_gpio.h"
#include "mock_flash.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

// Test utilities for SNMP Agent testing
class TestUtils {
public:
    // Packet generation helpers
    static std::vector<uint8_t> createSNMPGetRequest(
        const std::string& community,
        const std::vector<uint32_t>& oid,
        uint32_t requestID = 1
    ) {
        // Basic SNMP v1 GetRequest structure
        std::vector<uint8_t> packet = {
            0x30, 0x00,  // SEQUENCE, length placeholder
            0x02, 0x01, 0x00,  // Version: 1
        };
        
        // Add community string
        packet.push_back(0x04);  // OCTET STRING tag
        packet.push_back(community.length());  // Length
        packet.insert(packet.end(), community.begin(), community.end());
        
        // Start GetRequest PDU
        packet.push_back(0xA0);  // GetRequest tag
        packet.push_back(0x00);  // Length placeholder
        
        // Request ID
        packet.push_back(0x02);  // INTEGER tag
        std::vector<uint8_t> reqID = encodeInteger(requestID);
        packet.push_back(reqID.size());
        packet.insert(packet.end(), reqID.begin(), reqID.end());
        
        // Error status: 0
        packet.push_back(0x02);
        packet.push_back(0x01);
        packet.push_back(0x00);
        
        // Error index: 0
        packet.push_back(0x02);
        packet.push_back(0x01);
        packet.push_back(0x00);
        
        // Variable bindings
        packet.push_back(0x30);  // SEQUENCE
        packet.push_back(0x00);  // Length placeholder
        
        // OID binding
        packet.push_back(0x30);  // SEQUENCE
        packet.push_back(0x00);  // Length placeholder
        
        // Object identifier
        packet.push_back(0x06);  // OBJECT IDENTIFIER tag
        std::vector<uint8_t> encodedOID = encodeOID(oid);
        packet.push_back(encodedOID.size());
        packet.insert(packet.end(), encodedOID.begin(), encodedOID.end());
        
        // NULL value
        packet.push_back(0x05);
        packet.push_back(0x00);
        
        // Fix up lengths
        fixLength(packet);
        
        return packet;
    }
    
    // Memory tracking helpers
    static void startMemoryTracking() {
        initialFreeRam = getFreeRam();
        peakRamUsage = 0;
    }
    
    static size_t getMemoryUsage() {
        size_t currentRam = getFreeRam();
        size_t ramUsed = initialFreeRam - currentRam;
        peakRamUsage = std::max(peakRamUsage, ramUsed);
        return ramUsed;
    }
    
    static size_t getPeakMemoryUsage() {
        return peakRamUsage;
    }
    
    // Timing measurement helpers
    static void startTiming() {
        startTime = micros();
    }
    
    static unsigned long getElapsedMicros() {
        return micros() - startTime;
    }
    
    static unsigned long getElapsedMillis() {
        return (micros() - startTime) / 1000;
    }
    
    // Hex dump helper for debugging
    static std::string hexDump(const uint8_t* data, size_t length) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        
        for (size_t i = 0; i < length; i++) {
            if (i > 0) ss << ' ';
            ss << std::setw(2) << static_cast<int>(data[i]);
        }
        
        return ss.str();
    }
    
private:
    static size_t initialFreeRam;
    static size_t peakRamUsage;
    static unsigned long startTime;
    
    // Get available RAM (from mock_flash.h)
    static size_t getFreeRam() {
        extern char __heap_start[];
        extern char __heap_end[];
        extern char *__brkval;
        char *heap_end = (__brkval == 0 ? __heap_start : __brkval);
        return __heap_end - heap_end;
    }
    
    // Helper to encode an integer
    static std::vector<uint8_t> encodeInteger(uint32_t value) {
        std::vector<uint8_t> result;
        if (value < 128) {
            result.push_back(value);
        } else if (value < 32768) {
            result.push_back((value >> 8) & 0xFF);
            result.push_back(value & 0xFF);
        } else {
            result.push_back((value >> 24) & 0xFF);
            result.push_back((value >> 16) & 0xFF);
            result.push_back((value >> 8) & 0xFF);
            result.push_back(value & 0xFF);
        }
        return result;
    }
    
    // Helper to encode an OID
    static std::vector<uint8_t> encodeOID(const std::vector<uint32_t>& oid) {
        std::vector<uint8_t> result;
        if (oid.size() >= 2) {
            // First two numbers are encoded as: 40 * X + Y
            result.push_back(40 * oid[0] + oid[1]);
            
            // Encode remaining numbers
            for (size_t i = 2; i < oid.size(); i++) {
                uint32_t value = oid[i];
                if (value < 128) {
                    result.push_back(value);
                } else {
                    std::vector<uint8_t> bytes;
                    while (value > 0) {
                        bytes.insert(bytes.begin(), (value & 0x7F) | (bytes.empty() ? 0x00 : 0x80));
                        value >>= 7;
                    }
                    result.insert(result.end(), bytes.begin(), bytes.end());
                }
            }
        }
        return result;
    }
    
    // Helper to fix up ASN.1 lengths
    static void fixLength(std::vector<uint8_t>& packet) {
        // Fix sequence lengths from innermost to outermost
        std::vector<size_t> sequenceStarts;
        for (size_t i = 0; i < packet.size(); i++) {
            if (packet[i] == 0x30 || packet[i] == 0xA0) {
                sequenceStarts.push_back(i);
            }
        }
        
        // Process sequences in reverse order
        for (auto it = sequenceStarts.rbegin(); it != sequenceStarts.rend(); ++it) {
            size_t start = *it;
            size_t contentStart = start + 2;  // Skip tag and length byte
            size_t contentLength = 0;
            
            // Find next sequence start or end of packet
            size_t nextStart = packet.size();
            if (std::next(it) != sequenceStarts.rend()) {
                nextStart = *std::next(it);
            }
            
            contentLength = nextStart - contentStart;
            packet[start + 1] = contentLength;
        }
    }
};

// Initialize static members
size_t TestUtils::initialFreeRam = 0;
size_t TestUtils::peakRamUsage = 0;
unsigned long TestUtils::startTime = 0;

#endif // TEST_UTILS_H
