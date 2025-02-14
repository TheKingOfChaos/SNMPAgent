#include <Arduino.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "W5500.h"
#include "UDPStack.h"
#include "Settings.h"
#include "FactoryReset.h"
#include "SerialCom.h"
#include "CLI.h"

// Pin Definitions
const uint8_t POWER_MONITOR_PIN = 27;  // GPIO27 for mains power detection
const uint8_t LED_PIN = LED_BUILTIN;

// W5500 Pin Configuration
const uint8_t W5500_MISO = 16;
const uint8_t W5500_CS   = 17;
const uint8_t W5500_SCK  = 18;
const uint8_t W5500_MOSI = 19;
const uint8_t W5500_RST  = 20;
const uint8_t W5500_INT  = 21;

// Default MAC Address
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Network objects
W5500 eth(W5500_CS, W5500_RST, W5500_INT);
UDPStack udp(eth);

// Global objects
SettingsManager settings;
FactoryResetHandler factoryReset(settings);
SerialCom serial;
CLI cliHandler(serial, settings);

// Core 1 entry point - handles GPIO and power monitoring
void core1_entry() {
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
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    
    // Initialize serial communication
    serial.init();
    serial.sendln("SNMP Client Starting...");
    
    // Load settings before network initialization
    if (!settings.loadSettings()) {
        serial.sendln("Using default settings");
    }
    
    // Configure SPI pins for W5500
    pinMode(W5500_MISO, INPUT);
    pinMode(W5500_MOSI, OUTPUT);
    pinMode(W5500_SCK, OUTPUT);
    
    // Initialize W5500
    if (!eth.begin()) {
        serial.sendln("Failed to initialize W5500!");
        while (1) delay(1000); // Halt if W5500 init fails
    }
    
    // Launch Core 1
    multicore_fifo_push_blocking(0);
    multicore_launch_core1(core1_entry);
    
    // Set MAC address
    eth.setMAC(mac);
    
    // Start DHCP if enabled in settings, otherwise use static IP
    const DeviceSettings& deviceSettings = settings.getSettings();
    if (deviceSettings.dhcpEnabled) {
        serial.sendln("Starting DHCP...");
        if (!udp.startDHCP()) {
            serial.sendln("DHCP failed! Check network connection.");
            while (1) delay(1000); // Halt if DHCP fails
        }
    } else {
        serial.sendln("Using static IP configuration...");
        eth.setIP(deviceSettings.staticIP);
        eth.setSubnet(deviceSettings.subnetMask);
        eth.setGateway(deviceSettings.gateway);
        if (!eth.begin()) {
            serial.sendln("Static IP configuration failed!");
            while (1) delay(1000);
        }
    }
    
    serial.sendln("Network initialization complete!");
    
    // Update uptime initially
    settings.updateUptime();
}

void loop() {
    cliHandler.process();
    
    // Check network connection
    if (!udp.isConnected()) {
        serial.sendln("Network connection lost! Attempting to reconnect...");
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
