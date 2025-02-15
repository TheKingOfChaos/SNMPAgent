#include <Arduino.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "W5500.h"
#include "UDPStack.h"
#include "Settings.h"
#include "FactoryReset.h"
#include "SerialCom.h"
#include "CLI.h"
#include "PowerMonitor.h"
#include "CircuitProtection.h"
#include "SecurityManager.h"
#include "ErrorHandler.h"
#include "MIB.h"
#include "SNMPMessage.h"
#include "SNMPAgent.h"

// Pin and Configuration Constants
const uint8_t POWER_MONITOR_PIN = 27;  // GPIO27 for mains power detection
const uint8_t LED_PIN = LED_BUILTIN;
const uint8_t FACTORY_RESET_PIN = 22;  // GPIO22 for factory reset

// W5500 Pin Configuration
const uint8_t W5500_MISO = 16;
const uint8_t W5500_CS   = 17;
const uint8_t W5500_SCK  = 18;
const uint8_t W5500_MOSI = 19;
const uint8_t W5500_RST  = 20;
const uint8_t W5500_INT  = 21;

// Default MAC Address
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Global objects
W5500 eth(W5500_CS, W5500_RST, W5500_INT);
UDPStack udp(eth);
SettingsManager settings;
FactoryResetHandler factoryReset(settings);
SerialCom serial;
CLI cliHandler(serial, settings);
MIB mib;
PowerMonitor powerMonitor(mib);
SecurityManager security(mib);
CircuitProtection circuitProtection;

// Error handling callback
void handleError(const ErrorHandler::ErrorInfo& error) {
    char buffer[128];
    char hexStr[8];
    unsigned int code = (unsigned int)(error.code & 0xFFFF);
    
    // Convert hex value to string manually
    hexStr[0] = '0';
    hexStr[1] = 'x';
    hexStr[2] = "0123456789ABCDEF"[(code >> 12) & 0xF];
    hexStr[3] = "0123456789ABCDEF"[(code >> 8) & 0xF];
    hexStr[4] = "0123456789ABCDEF"[(code >> 4) & 0xF];
    hexStr[5] = "0123456789ABCDEF"[code & 0xF];
    hexStr[6] = '\0';
    
    // Build message parts
    char timeStr[32];
    ltoa(error.timestamp, timeStr, 10);
    
    const char* severity;
    switch (error.severity) {
        case ErrorHandler::Severity::CRITICAL:
            severity = "CRITICAL";
            break;
        case ErrorHandler::Severity::ERROR:
            severity = "ERROR";
            break;
        case ErrorHandler::Severity::WARNING:
            severity = "WARNING";
            break;
        default:
            severity = "INFO";
    }
    
    // Build final message using pointer arithmetic to avoid buffer overflows
    char* ptr = buffer;
    *ptr++ = '[';
    strcpy(ptr, timeStr);
    ptr += strlen(timeStr);
    *ptr++ = ']';
    *ptr++ = ' ';
    strcpy(ptr, severity);
    ptr += strlen(severity);
    strcpy(ptr, ": ");
    ptr += 2;
    strcpy(ptr, error.message);
    ptr += strlen(error.message);
    strcpy(ptr, " (");
    ptr += 2;
    strcpy(ptr, hexStr);
    ptr += strlen(hexStr);
    *ptr++ = ')';
    *ptr = '\0';
    
    serial.sendln(buffer);
}

// Core 1 entry point - handles GPIO and power monitoring
void core1_entry() {
    // Configure circuit protection for power monitoring
    CircuitProtection::ProtectionConfig powerConfig = {
        .type = CircuitProtection::ProtectionType::ISOLATED_INPUT,
        .interruptMode = CircuitProtection::InterruptMode::BOTH_EDGES,
        .maxVoltage = 3.3f
    };
    circuitProtection.protectPin(POWER_MONITOR_PIN, powerConfig);
    
    // Configure circuit protection for factory reset
    CircuitProtection::ProtectionConfig resetConfig = {
        .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP,
        .interruptMode = CircuitProtection::InterruptMode::FALLING,
        .maxVoltage = 3.3f
    };
    circuitProtection.protectPin(FACTORY_RESET_PIN, resetConfig);
    
    // Initialize power monitoring
    powerMonitor.begin();
    
    while (true) {
        // Check factory reset button
        factoryReset.checkResetButton();
        
        // Monitor circuit protection status
        if (circuitProtection.hasErrors(POWER_MONITOR_PIN)) {
            REPORT_ERROR(ErrorHandler::Severity::WARNING,
                        ErrorHandler::Category::HARDWARE,
                        0x3001,
                        "Power monitoring circuit fault detected");
        }
        
        // Small delay to prevent tight looping
        delay(10); // 10ms for quick response to power changes
    }
}

void setup() {
    // Initialize error handling
    ErrorHandler::getInstance().registerCallback(handleError);
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    
    // Initialize serial communication
    serial.init();
    serial.sendln("SNMP Agent Starting...");
    
    // Initialize MIB
    mib.initialize();
    
    // Load settings before network initialization
    if (!settings.loadSettings()) {
        REPORT_WARNING(ErrorHandler::Category::SYSTEM, 0x1001, "Using default settings");
    }
    
    // Configure SPI pins for W5500
    pinMode(W5500_MISO, INPUT);
    pinMode(W5500_MOSI, OUTPUT);
    pinMode(W5500_SCK, OUTPUT);
    
    // Initialize W5500
    if (!eth.begin()) {
        REPORT_ERROR_CRITICAL(ErrorHandler::Category::NETWORK, 0x2001, "Failed to initialize W5500!");
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
            REPORT_ERROR_CRITICAL(ErrorHandler::Category::NETWORK, 0x2002, "DHCP failed! Check network connection.");
            while (1) delay(1000); // Halt if DHCP fails
        }
    } else {
        serial.sendln("Using static IP configuration...");
        eth.setIP(deviceSettings.staticIP);
        eth.setSubnet(deviceSettings.subnetMask);
        eth.setGateway(deviceSettings.gateway);
        if (!eth.begin()) {
            REPORT_ERROR_CRITICAL(ErrorHandler::Category::NETWORK, 0x2003, "Static IP configuration failed!");
            while (1) delay(1000);
        }
    }
    
    serial.sendln("Network initialization complete!");
    
    // Update uptime initially
    settings.updateUptime();
}

void loop() {
    // Process CLI commands
    cliHandler.process();
    
    // Check network connection
    if (!udp.isConnected()) {
        REPORT_ERROR(ErrorHandler::Severity::ERROR,
                    ErrorHandler::Category::NETWORK,
                    0x2004,
                    "Network connection lost! Attempting to reconnect...");
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
    
    // Process SNMP messages
    SNMPAgent::processMessages(udp, security, mib);
    
    // Check system health
    if (!ErrorHandler::getInstance().isSystemHealthy()) {
        digitalWrite(LED_PIN, HIGH);  // Light LED to indicate issues
    } else {
        digitalWrite(LED_PIN, LOW);
    }
    
    delay(10); // Small delay to prevent tight looping
}
