#pragma once

#include <Arduino.h>
#include <hardware/flash.h>

// Flash storage constants
constexpr uint32_t SETTINGS_FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - (1024 * 1024); // 1MB from end of flash
constexpr uint32_t SETTINGS_BLOCK_SIZE = FLASH_SECTOR_SIZE;  // Use flash sector size
constexpr uint32_t SETTINGS_NUM_BLOCKS = 8;             // 8 blocks for wear leveling
constexpr uint32_t SETTINGS_MAGIC = 0x534E4D50;         // "SNMP" in ASCII
constexpr uint32_t SETTINGS_VERSION = 1;                // Initial version
constexpr uint32_t SETTINGS_FOOTER = 0x454E44;          // "END" in ASCII

// Settings data structure
struct DeviceSettings {
    // Network Configuration
    bool dhcpEnabled;                    // 1 byte
    uint8_t staticIP[4];                 // 4 bytes
    uint8_t subnetMask[4];              // 4 bytes
    uint8_t gateway[4];                  // 4 bytes
    
    // SNMP Configuration
    char communityString[32];            // 32 bytes
    uint16_t snmpPort;                   // 2 bytes
    uint32_t rateLimit;                  // 4 bytes
    
    // Device Statistics
    uint32_t powerLossCount;             // 4 bytes
    uint32_t uptime;                     // 4 bytes
    uint32_t lastPowerLoss;              // 4 bytes
    
    // Reserved space for future expansion
    uint8_t reserved[64];                // 64 bytes
};

// Block header structure
struct BlockHeader {
    uint32_t magic;
    uint32_t crc32;
    uint16_t version;
    uint16_t dataLength;
};

// Block footer structure
struct BlockFooter {
    uint32_t marker;
};

class SettingsManager {
public:
    SettingsManager();
    
    // Core operations
    bool loadSettings();
    bool saveSettings();
    bool factoryReset();
    bool validateSettings();
    
    // Getters and setters
    const DeviceSettings& getSettings() const { return currentSettings; }
    bool updateSettings(const DeviceSettings& newSettings);
    
    // Individual setting updates
    bool setDHCP(bool enabled);
    bool setStaticIP(const uint8_t ip[4]);
    bool setSubnetMask(const uint8_t mask[4]);
    bool setGateway(const uint8_t gw[4]);
    bool setCommunityString(const char* community);
    bool setSNMPPort(uint16_t port);
    bool setRateLimit(uint32_t limit);
    
    // Statistics updates
    void incrementPowerLossCount();
    void updateUptime();
    void recordPowerLoss();

private:
    DeviceSettings currentSettings;
    uint32_t activeBlock;
    
    // Internal helper functions
    bool findLatestBlock();
    bool writeBlock(const DeviceSettings& settings, uint32_t blockIndex);
    bool readBlock(DeviceSettings& settings, uint32_t blockIndex);
    uint32_t calculateCRC32(const void* data, size_t length);
    void initializeDefaultSettings();
    bool isBlockValid(uint32_t blockIndex);
    bool eraseBlock(uint32_t blockIndex);
    uint32_t getNextBlockIndex();
    
    // Flash memory helpers
    bool writeToFlash(uint32_t offset, const void* data, size_t length);
    bool readFromFlash(uint32_t offset, void* data, size_t length);
};
