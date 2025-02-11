#ifndef W5500_H
#define W5500_H

#include <Arduino.h>
#include <SPI.h>

// W5500 SPI operation modes
#define W5500_READ        0x00
#define W5500_WRITE       0x04

// W5500 Register Addresses
#define MR               0x0000    // Mode Register
#define GAR              0x0001    // Gateway Address Register
#define SUBR             0x0005    // Subnet Mask Register
#define SHAR             0x0009    // Source Hardware Address Register
#define SIPR             0x000F    // Source IP Address Register
#define PHYCFGR          0x002E    // PHY Configuration Register

class W5500 {
public:
    W5500(uint8_t cs_pin, uint8_t rst_pin, uint8_t int_pin);
    bool begin();
    void reset();
    bool isLinked();
    
    // Network configuration
    void setMAC(const uint8_t* mac_addr);
    void setIP(const uint8_t* ip_addr);
    void setGateway(const uint8_t* gw_addr);
    void setSubnet(const uint8_t* subnet);
    
    // DHCP functions
    bool startDHCP();
    bool renewDHCP();
    void stopDHCP();
    
    // UDP functions
    bool beginPacket(const uint8_t* ip, uint16_t port);
    bool endPacket();
    size_t write(const uint8_t* buffer, size_t size);
    int parsePacket();
    int read(uint8_t* buffer, size_t size);
    
private:
    uint8_t _cs_pin;
    uint8_t _rst_pin;
    uint8_t _int_pin;
    bool _dhcp_enabled;
    
    // SPI operations
    void writeRegister(uint16_t addr, uint8_t data);
    void writeRegisters(uint16_t addr, const uint8_t* data, size_t len);
    uint8_t readRegister(uint16_t addr);
    void readRegisters(uint16_t addr, uint8_t* data, size_t len);
    
    // Internal helper functions
    void selectChip();
    void deselectChip();
    bool verifyLink();
    void handleInterrupt();
};

#endif // W5500_H
