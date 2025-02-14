#include <Arduino.h>
#include "W5500.h"
#include "UDPStack.h"
#include "Settings.h"
#include "FactoryReset.h"
#include "pico/multicore.h"

// GPIO Configuration (from .clinerules)
#define POWER_MONITOR_PIN 27  // GPIO27 for mains power detection

// W5500 Pin Configuration
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

// Global objects
SettingsManager settings;
FactoryResetHandler factoryReset(settings);

// Core 1 entry point - handles GPIO and power monitoring
void core1_main() {
    // Initialize power monitoring pin
    pinMode(POWER_MONITOR_PIN, INPUT);
    
    while (true) {
        // Check factory reset button
        factoryReset.checkResetButton();
        
        // Monitor mains power
        bool powerPresent = digitalRead(POWER_MONITOR_PIN);
        static bool lastPowerState = true;
        
        if (powerPresent != lastPowerState) {
            if (!powerPresent) {
                settings.recordPowerLoss();
            }
            lastPowerState = powerPresent;
        }
        
        // Small delay to prevent tight looping
        delay(10); // 10ms for quick response to power changes
    }
}

void setup() {
    // Load settings before network initialization
    if (!settings.loadSettings()) {
        Serial.println("Using default settings");
    }
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
    
    // Launch Core 1
    multicore_launch_core1(core1_main);
    
    // Set MAC address
    eth.setMAC(mac);
    
    // Start DHCP if enabled in settings, otherwise use static IP
    const DeviceSettings& deviceSettings = settings.getSettings();
    if (deviceSettings.dhcpEnabled) {
        Serial.println("Starting DHCP...");
        if (!udp.startDHCP()) {
            Serial.println("DHCP failed! Check network connection.");
            while (1) delay(1000); // Halt if DHCP fails
        }
    } else {
        Serial.println("Using static IP configuration...");
        eth.setIPAddress(deviceSettings.staticIP);
        eth.setSubnetMask(deviceSettings.subnetMask);
        eth.setGateway(deviceSettings.gateway);
        if (!eth.begin()) {
            Serial.println("Static IP configuration failed!");
            while (1) delay(1000);
        }
    }
    
    Serial.println("Network initialization complete!");
    
    // Update uptime periodically
    settings.updateUptime();
}

void loop() {
    // Core 0 handles network and SNMP
    
    // Check network connection
    if (!udp.isConnected()) {
        Serial.println("Network connection lost! Attempting to reconnect...");
        const DeviceSettings& deviceSettings = settings.getSettings();
        if (deviceSettings.dhcpEnabled) {
            udp.startDHCP();
        }
        delay(1000);
        return;
    }
    
    // Handle any pending DHCP renewals if DHCP is enabled
    if (settings.getSettings().dhcpEnabled) {
        udp.renewDHCP();
    }
    
    // Update device uptime periodically
    static uint32_t lastUptimeUpdate = 0;
    uint32_t currentTime = millis();
    if (currentTime - lastUptimeUpdate >= 60000) { // Update every minute
        settings.updateUptime();
        lastUptimeUpdate = currentTime;
    }
    
    // Main loop processing will be added here
    // - SNMP message handling
    // - Network monitoring
    // - Status updates
    
    delay(100); // Small delay to prevent tight looping
}
