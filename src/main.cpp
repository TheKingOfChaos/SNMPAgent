#include <Arduino.h>
#include "W5500.h"
#include "UDPStack.h"

// W5500 Pin Configuration (from .clinerules)
#define W5500_MISO     16
#define W5500_CS       17
#define W5500_SCK      18
#define W5500_MOSI     19
#define W5500_RST      20
#define W5500_INT      21

// Default MAC Address
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Network objects
W5500 eth(W5500_CS, W5500_RST, W5500_INT);
UDPStack udp(eth);

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    Serial.println("SNMP Client Starting...");
    
    // Configure SPI pins for W5500
    pinMode(W5500_MISO, INPUT);
    pinMode(W5500_MOSI, OUTPUT);
    pinMode(W5500_SCK, OUTPUT);
    
    // Initialize W5500
    if (!eth.begin()) {
        Serial.println("Failed to initialize W5500!");
        while (1) delay(1000); // Halt if W5500 init fails
    }
    
    // Set MAC address
    eth.setMAC(mac);
    
    // Start DHCP (default configuration)
    Serial.println("Starting DHCP...");
    if (!udp.startDHCP()) {
        Serial.println("DHCP failed! Check network connection.");
        while (1) delay(1000); // Halt if DHCP fails
    }
    
    Serial.println("Network initialization complete!");
}

void loop() {
    // Check network connection
    if (!udp.isConnected()) {
        Serial.println("Network connection lost! Attempting to reconnect...");
        udp.startDHCP();
        delay(1000);
        return;
    }
    
    // Handle any pending DHCP renewals
    udp.renewDHCP();
    
    // Main loop processing will be added here
    // - SNMP message handling
    // - Network monitoring
    // - Status updates
    
    delay(100); // Small delay to prevent tight looping
}
