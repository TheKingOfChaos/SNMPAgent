#include "W5500.h"

W5500::W5500(uint8_t cs_pin, uint8_t rst_pin, uint8_t int_pin)
    : _cs_pin(cs_pin)
    , _rst_pin(rst_pin)
    , _int_pin(int_pin)
    , _dhcp_enabled(false) {
}

bool W5500::begin() {
    // Configure pins
    pinMode(_cs_pin, OUTPUT);
    pinMode(_rst_pin, OUTPUT);
    pinMode(_int_pin, INPUT);
    digitalWrite(_cs_pin, HIGH);
    
    // Initialize SPI
    SPI.begin();
    SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
    
    // Reset the W5500
    reset();
    
    // Verify chip is responding
    uint8_t version = readRegister(0x0039);  // Version register
    if (version != 0x04) {  // W5500 version should be 0x04
        return false;
    }
    
    // Initialize chip
    writeRegister(MR, 0x80);  // Soft reset
    delay(1);
    
    return true;
}

void W5500::reset() {
    digitalWrite(_rst_pin, LOW);
    delay(1);
    digitalWrite(_rst_pin, HIGH);
    delay(1);
}

bool W5500::isLinked() {
    return (readRegister(PHYCFGR) & 0x01) == 0x01;
}

void W5500::setMAC(const uint8_t* mac_addr) {
    writeRegisters(SHAR, mac_addr, 6);
}

void W5500::setIP(const uint8_t* ip_addr) {
    writeRegisters(SIPR, ip_addr, 4);
}

void W5500::setGateway(const uint8_t* gw_addr) {
    writeRegisters(GAR, gw_addr, 4);
}

void W5500::setSubnet(const uint8_t* subnet) {
    writeRegisters(SUBR, subnet, 4);
}

void W5500::writeRegister(uint16_t addr, uint8_t data) {
    selectChip();
    
    // Send address
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    
    // Send control byte (BSB = 000, RWB = 1 for write)
    SPI.transfer(W5500_WRITE);
    
    // Send data
    SPI.transfer(data);
    
    deselectChip();
}

void W5500::writeRegisters(uint16_t addr, const uint8_t* data, size_t len) {
    selectChip();
    
    // Send address
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    
    // Send control byte
    SPI.transfer(W5500_WRITE);
    
    // Send data
    for (size_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    
    deselectChip();
}

uint8_t W5500::readRegister(uint16_t addr) {
    selectChip();
    
    // Send address
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    
    // Send control byte (BSB = 000, RWB = 0 for read)
    SPI.transfer(W5500_READ);
    
    // Read data
    uint8_t data = SPI.transfer(0);
    
    deselectChip();
    return data;
}

void W5500::readRegisters(uint16_t addr, uint8_t* data, size_t len) {
    selectChip();
    
    // Send address
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    
    // Send control byte
    SPI.transfer(W5500_READ);
    
    // Read data
    for (size_t i = 0; i < len; i++) {
        data[i] = SPI.transfer(0);
    }
    
    deselectChip();
}

void W5500::selectChip() {
    digitalWrite(_cs_pin, LOW);
}

void W5500::deselectChip() {
    digitalWrite(_cs_pin, HIGH);
}

bool W5500::verifyLink() {
    return isLinked();
}

void W5500::handleInterrupt() {
    // TODO: Implement interrupt handling for various W5500 events
}

// UDP Functions
bool W5500::beginPacket(const uint8_t* ip, uint16_t port) {
    // TODO: Implement UDP packet initialization
    return true;
}

bool W5500::endPacket() {
    // TODO: Implement UDP packet transmission
    return true;
}

size_t W5500::write(const uint8_t* buffer, size_t size) {
    // TODO: Implement UDP data writing
    return size;
}

int W5500::parsePacket() {
    // TODO: Implement UDP packet parsing
    return 0;
}

int W5500::read(uint8_t* buffer, size_t size) {
    // TODO: Implement UDP data reading
    return 0;
}

// DHCP Functions
bool W5500::startDHCP() {
    // TODO: Implement DHCP client
    _dhcp_enabled = true;
    return true;
}

bool W5500::renewDHCP() {
    // TODO: Implement DHCP renewal
    return _dhcp_enabled;
}

void W5500::stopDHCP() {
    _dhcp_enabled = false;
}
