# SNMP Agent Implementation TODO List

## 1. Core Network Stack Implementation
- [x] Implement W5500 driver
  - [x] Set up SPI communication (GPIO16-21)
  - [x] Implement basic read/write operations
  - [x] Add interrupt handling for GPIO21
- [x] Create custom UDP/IP stack
  - [x] Implement UDP packet handling
  - [x] Add IP addressing functionality
  - [x] Implement DHCP client
  - [x] Add static IP configuration support

## 2. SNMP Protocol Implementation
- [ ] Implement ASN.1/BER encoding (no external libraries)
  - [ ] Add INTEGER encoding/decoding
  - [ ] Add OCTET STRING encoding/decoding
  - [ ] Add OID encoding/decoding
  - [ ] Add NULL encoding/decoding
  - [ ] Add SEQUENCE encoding/decoding
  - [ ] Add length encoding/decoding
  - [ ] Add type encoding/decoding
  - [ ] Implement BER encoding validation
- [ ] Create SNMP v1 message handling
  - [ ] Implement PDU structure
  - [ ] Implement message parser
  - [ ] Add community string validation (default: "public")
  - [ ] Add community string CLI configuration
  - [ ] Implement GetRequest handler (100ms max response)
  - [ ] Implement GetNextRequest handler (100ms max response)
  - [ ] Add request rate limiting
  - [ ] Add malformed packet detection
  - [ ] Implement error response handling
- [ ] Implement MIB structure
  - [ ] Add system group (.1.3.6.1.2.1.1)
    - [ ] Add sysDescr
    - [ ] Add sysObjectID
    - [ ] Add sysUpTime
    - [ ] Add sysContact
    - [ ] Add sysName
    - [ ] Add sysLocation
    - [ ] Add sysServices
  - [ ] Add private group (.1.3.6.1.4.1.63050)
    - [ ] Add power monitoring OIDs (.1.3.6.1.4.1.63050.1)
    - [ ] Add configuration OIDs
    - [ ] Add statistics OIDs
  - [ ] Implement OID tree traversal for GetNext
    - [ ] Add lexicographic ordering
    - [ ] Handle end-of-MIB condition
- [ ] Add SNMP testing framework
  - [ ] Test community string validation
  - [ ] Test GetRequest operations
  - [ ] Test GetNextRequest operations
  - [ ] Test MIB walk functionality
  - [ ] Test response timing requirements
  - [ ] Test error conditions
  - [ ] Test malformed packets

## 3. Power Monitoring System
- [ ] Set up GPIO27 for power detection
  - [ ] Configure pin as input with proper pull-up/down
  - [ ] Implement interrupt handling for state changes
  - [ ] Add debouncing for reliable detection
- [ ] Create power status monitoring
  - [ ] Implement status checking function
  - [ ] Add status to MIB structure
  - [ ] Create status change notification system

## 4. Configuration System
- [x] Implement flash storage system
  - [x] Add wear leveling algorithm (8-block rotation system)
  - [x] Implement data integrity validation (CRC32 + magic numbers)
  - [x] Create configuration structure (network, SNMP, and statistics)
- [x] Add factory reset functionality
  - [x] Set up GPIO22 for reset button
  - [x] Implement 10-second hold detection
  - [x] Add LED feedback system
  - [x] Create settings reset procedure

## 5. CLI Interface
- [x] Create serial communication system
  - [x] Set up UART (115200 baud)
  - [x] Implement command parser
  - [x] Add input sanitization
- [x] Implement menu system
  - [x] Add status display
  - [x] Create network configuration options
  - [x] Add SNMP configuration
  - [x] Implement statistics viewing
  - [x] Add configuration save option

## 6. Testing & Validation
- [ ] Create hardware test suite
  - [ ] Test power monitoring circuit
  - [ ] Validate GPIO configurations
  - [ ] Test W5500 communication
  - [ ] Verify factory reset functionality
- [ ] Implement SNMP testing
  - [ ] Test GetRequest operations
  - [ ] Validate GetNextRequest functionality
  - [ ] Test community string validation
  - [ ] Verify MIB walk operations
- [ ] Performance testing
  - [ ] Measure SNMP response times
  - [ ] Verify CLI responsiveness
  - [ ] Test power state detection speed
  - [ ] Validate factory reset timing

## 7. Documentation
- [ ] Code documentation
  - [ ] Add function documentation
  - [ ] Document MIB structure
  - [ ] Add hardware connection details
  - [ ] Document configuration options
- [ ] User documentation
  - [ ] Create CLI command guide
  - [ ] Add network configuration guide
  - [ ] Provide SNMP usage examples
  - [ ] Create troubleshooting guide

## 8. Safety & Security Implementation
- [ ] Implement security measures
  - [ ] Add request rate limiting
  - [ ] Implement access logging
  - [ ] Add buffer overflow protection
  - [ ] Implement packet validation
- [ ] Add circuit protection
  - [ ] Implement GPIO protection
  - [ ] Add power monitoring isolation
  - [ ] Verify voltage levels

## 9. Build System & Quality Control
- [ ] Set up PlatformIO configuration
  - [ ] Configure build environment
  - [ ] Set up dependency management
  - [ ] Add build documentation
- [ ] Implement error handling
  - [ ] Add global error handling system
  - [ ] Implement recovery procedures
  - [ ] Add error logging

## Priority Order
1. Core Network Stack (required for basic communication)
2. SNMP Protocol (core functionality)
3. Configuration System (required for persistence)
4. Power Monitoring (main feature)
5. CLI Interface (user configuration)
6. Safety & Security (critical for deployment)
7. Testing & Validation
8. Documentation
9. Build System & Quality Control

## Notes
- All GPIO inputs must include proper protection circuits
- Network stack must be implemented without external libraries
- SNMP implementation must be v1 only
- Memory usage must not exceed specified limits (70% flash, 50% RAM)
- All changes must be documented and version controlled
