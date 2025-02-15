#ifndef MOCK_FLASH_H
#define MOCK_FLASH_H

#include <vector>
#include <cstring>
#include <cstdint>
#include <Arduino.h>

// Mock Flash memory interface for testing
class MockFlash {
public:
    // Flash sector size (4KB for RP2040)
    static constexpr size_t SECTOR_SIZE = 4096;
    
    // Total flash size (2MB for RP2040)
    static constexpr size_t TOTAL_SIZE = 2 * 1024 * 1024;
    
    // Number of sectors
    static constexpr size_t SECTOR_COUNT = TOTAL_SIZE / SECTOR_SIZE;
    
    MockFlash() {
        // Initialize flash memory with 0xFF (erased state)
        memory.resize(TOTAL_SIZE, 0xFF);
        sectorEraseCount.resize(SECTOR_COUNT, 0);
    }
    
    // Read data from flash
    bool read(uint32_t address, uint8_t* buffer, size_t length) {
        if (!isValidAccess(address, length)) {
            return false;
        }
        
        memcpy(buffer, &memory[address], length);
        return true;
    }
    
    // Write data to flash (can only write to erased (0xFF) locations)
    bool write(uint32_t address, const uint8_t* data, size_t length) {
        if (!isValidAccess(address, length)) {
            return false;
        }
        
        // Check if all bytes are erased
        for (size_t i = 0; i < length; i++) {
            if (memory[address + i] != 0xFF) {
                return false; // Can't write to non-erased location
            }
        }
        
        memcpy(&memory[address], data, length);
        return true;
    }
    
    // Erase a sector (set to 0xFF)
    bool eraseSector(uint32_t sector) {
        if (sector >= SECTOR_COUNT) {
            return false;
        }
        
        uint32_t address = sector * SECTOR_SIZE;
        memset(&memory[address], 0xFF, SECTOR_SIZE);
        sectorEraseCount[sector]++;
        return true;
    }
    
    // Test helper methods
    size_t getEraseCount(uint32_t sector) {
        return (sector < SECTOR_COUNT) ? sectorEraseCount[sector] : 0;
    }
    
    bool isErased(uint32_t address, size_t length) {
        if (!isValidAccess(address, length)) {
            return false;
        }
        
        for (size_t i = 0; i < length; i++) {
            if (memory[address + i] != 0xFF) {
                return false;
            }
        }
        return true;
    }
    
    // Get wear leveling statistics
    void getWearStats(size_t& minEraseCount, size_t& maxEraseCount, float& avgEraseCount) {
        minEraseCount = SIZE_MAX;
        maxEraseCount = 0;
        size_t totalErases = 0;
        
        for (size_t count : sectorEraseCount) {
            minEraseCount = std::min(minEraseCount, count);
            maxEraseCount = std::max(maxEraseCount, count);
            totalErases += count;
        }
        
        avgEraseCount = static_cast<float>(totalErases) / SECTOR_COUNT;
    }
    
    // Reset flash to initial state
    void reset() {
        std::fill(memory.begin(), memory.end(), 0xFF);
        std::fill(sectorEraseCount.begin(), sectorEraseCount.end(), 0);
    }
    
    // Simulate flash corruption
    void corruptData(uint32_t address, size_t length) {
        if (isValidAccess(address, length)) {
            for (size_t i = 0; i < length; i++) {
                memory[address + i] ^= 0xFF; // Flip all bits
            }
        }
    }
    
private:
    std::vector<uint8_t> memory;
    std::vector<size_t> sectorEraseCount;
    
    bool isValidAccess(uint32_t address, size_t length) {
        return (address < TOTAL_SIZE) && (address + length <= TOTAL_SIZE);
    }
};

#endif // MOCK_FLASH_H
