#include "CLI.h"
#include <string.h>

CLI::CLI(SerialCom& serial, SettingsManager& settings)
    : serialCom(serial), settings(settings), bufferIndex(0) {
    memset(commandBuffer, 0, MAX_COMMAND_LENGTH);
    serialCom.send("> "); // Initial prompt
}

void CLI::process() {
    while (serialCom.dataAvailable()) {
        char c = serialCom.read_char();
        
        // Handle backspace
        if (c == '\b' || c == 0x7F) {
            if (bufferIndex > 0) {
                bufferIndex--;
                serialCom.printf("\b \b"); // Erase character from terminal
            }
            continue;
        }
        
        // Handle enter key
        if (c == '\r' || c == '\n') {
            serialCom.sendln(""); // New line
            if (bufferIndex > 0) {
                commandBuffer[bufferIndex] = '\0';
                parseCommand();
                clearBuffer();
            }
            serialCom.send("> "); // Print prompt
            continue;
        }
        
        // Store character if there's space
        if (bufferIndex < MAX_COMMAND_LENGTH - 1) {
            commandBuffer[bufferIndex++] = c;
            char str[2] = {c, '\0'};
            serialCom.send(str); // Echo character
        }
    }
}

void CLI::parseCommand() {
    char* argv[MAX_ARGS];
    int argc = 0;
    
    if (!splitArgs(commandBuffer, argv, argc)) {
        printError("Invalid command format");
        return;
    }
    
    if (argc == 0) return;
    
    // Command matching
    if (strcmp(argv[0], "help") == 0) {
        handleHelp();
    }
    else if (strcmp(argv[0], "set") == 0) {
        if (argc < 3) {
            printError("Usage: set <option> <value>");
            return;
        }
        
        if (strcmp(argv[1], "community") == 0) {
            handleSetCommunity(argv[2]);
        }
        else if (strcmp(argv[1], "network") == 0) {
            handleSetNetwork(argc - 2, &argv[2]);
        }
        else {
            printError("Unknown setting");
        }
    }
    else if (strcmp(argv[0], "status") == 0) {
        handleStatus();
    }
    else if (strcmp(argv[0], "factory-reset") == 0) {
        handleFactoryReset();
    }
    else {
        printError("Unknown command. Type 'help' for available commands.");
    }
}

bool CLI::splitArgs(char* input, char* argv[], int& argc) {
    argc = 0;
    char* token = strtok(input, " ");
    
    while (token && static_cast<size_t>(argc) < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    return static_cast<size_t>(argc) <= MAX_ARGS;
}

void CLI::handleHelp() {
    serialCom.sendln("Available commands:");
    printCommandHelp("help", "help", "Show this help message");
    printCommandHelp("set community", "set community <string>", "Set SNMP community string");
    printCommandHelp("set network", "set network <dhcp|static> [ip] [mask] [gateway]", "Configure network settings");
    printCommandHelp("status", "status", "Show current device status");
    printCommandHelp("factory-reset", "factory-reset", "Reset device to factory settings");
}

void CLI::handleSetCommunity(const char* community) {
    if (!isValidCommunity(community)) {
        printError("Invalid community string (1-31 chars, alphanumeric and -_)");
        return;
    }
    
    if (settings.setCommunityString(community)) {
        if (settings.saveSettings()) {
            printSuccess("Community string updated");
        } else {
            printError("Failed to save settings");
        }
    } else {
        printError("Failed to update community string");
    }
}

void CLI::handleSetNetwork(int argc, char* argv[]) {
    if (argc < 1) {
        printError("Missing network mode");
        return;
    }
    
    DeviceSettings currentSettings = settings.getSettings();
    bool updated = false;
    
    if (strcmp(argv[0], "dhcp") == 0) {
        currentSettings.dhcpEnabled = true;
        updated = true;
    }
    else if (strcmp(argv[0], "static") == 0) {
        if (argc != 4) {
            printError("Usage: set network static <ip> <mask> <gateway>");
            return;
        }
        
        if (!isValidIPAddress(argv[1]) || !isValidIPAddress(argv[2]) || !isValidIPAddress(argv[3])) {
            printError("Invalid IP address format");
            return;
        }
        
        // Convert string IPs to byte arrays
        uint8_t ip[4], mask[4], gw[4];
        sscanf(argv[1], "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
        sscanf(argv[2], "%hhu.%hhu.%hhu.%hhu", &mask[0], &mask[1], &mask[2], &mask[3]);
        sscanf(argv[3], "%hhu.%hhu.%hhu.%hhu", &gw[0], &gw[1], &gw[2], &gw[3]);
        
        currentSettings.dhcpEnabled = false;
        memcpy(currentSettings.staticIP, ip, 4);
        memcpy(currentSettings.subnetMask, mask, 4);
        memcpy(currentSettings.gateway, gw, 4);
        updated = true;
    }
    else {
        printError("Invalid network mode. Use 'dhcp' or 'static'");
        return;
    }
    
    if (updated) {
        if (settings.updateSettings(currentSettings) && settings.saveSettings()) {
            printSuccess("Network settings updated");
        } else {
            printError("Failed to save network settings");
        }
    }
}

void CLI::handleStatus() {
    const DeviceSettings& config = settings.getSettings();
    
    serialCom.sendln("\nDevice Status:");
    serialCom.printf("Community String: %s\n", config.communityString);
    serialCom.printf("Network Mode: %s\n", config.dhcpEnabled ? "DHCP" : "Static");
    
    if (!config.dhcpEnabled) {
        serialCom.printf("IP Address: %d.%d.%d.%d\n", 
            config.staticIP[0], config.staticIP[1], config.staticIP[2], config.staticIP[3]);
        serialCom.printf("Subnet Mask: %d.%d.%d.%d\n",
            config.subnetMask[0], config.subnetMask[1], config.subnetMask[2], config.subnetMask[3]);
        serialCom.printf("Gateway: %d.%d.%d.%d\n",
            config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]);
    }
    
    serialCom.printf("SNMP Port: %d\n", config.snmpPort);
    serialCom.printf("Power Loss Count: %lu\n", config.powerLossCount);
    serialCom.printf("Uptime: %lu seconds\n", config.uptime);
    serialCom.sendln("");
}

void CLI::handleFactoryReset() {
    if (settings.factoryReset() && settings.saveSettings()) {
        printSuccess("Device reset to factory defaults");
    } else {
        printError("Failed to reset device");
    }
}

void CLI::clearBuffer() {
    memset(commandBuffer, 0, MAX_COMMAND_LENGTH);
    bufferIndex = 0;
}

bool CLI::isValidCommunity(const char* community) {
    if (!community || strlen(community) == 0 || strlen(community) > 31) {
        return false;
    }
    
    // Check for valid characters (alphanumeric, hyphen, underscore)
    for (const char* c = community; *c; c++) {
        if (!isalnum(*c) && *c != '-' && *c != '_') {
            return false;
        }
    }
    
    return true;
}

bool CLI::isValidIPAddress(const char* ip) {
    if (!ip) return false;
    
    int values[4];
    int parts = sscanf(ip, "%d.%d.%d.%d", &values[0], &values[1], &values[2], &values[3]);
    
    if (parts != 4) return false;
    
    for (int i = 0; i < 4; i++) {
        if (values[i] < 0 || values[i] > 255) return false;
    }
    
    return true;
}

void CLI::printError(const char* message) {
    serialCom.printf("Error: %s\n", message);
}

void CLI::printSuccess(const char* message) {
    serialCom.printf("Success: %s\n", message);
}

void CLI::printCommandHelp(const char* command, const char* usage, const char* description) {
    serialCom.printf("  %-15s - %s\n", command, description);
    serialCom.printf("    Usage: %s\n", usage);
}
