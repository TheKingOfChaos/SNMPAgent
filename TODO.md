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

### 2.1 Core ASN.1/BER Types (Foundation)
- [x] Basic Type System
  - [x] Create ASN.1 type definitions
  - [x] Implement type identification system
  - [x] Add length encoding/decoding
  - [x] Add tag encoding/decoding
- [x] Primitive Types
  - [x] INTEGER (includes Counter, Gauge)
  - [x] OCTET STRING
  - [x] NULL
  - [x] OBJECT IDENTIFIER
- [x] Constructed Types
  - [x] SEQUENCE
  - [x] SEQUENCE OF
- [x] Validation Layer
  - [x] Add type validation
  - [x] Add length validation
  - [x] Add bounds checking
  - [x] Unit tests for each type

### 2.2 SNMP Message Structure
- [x] Message Framework
  - [x] Define message structure classes
  - [x] Implement version handling (v1)
  - [x] Add community string support
- [x] PDU Components
  - [x] Request ID handling
  - [x] Error status/index
  - [x] Variable bindings
- [x] PDU Types
  - [x] GetRequest PDU
  - [x] GetNextRequest PDU
  - [x] GetResponse PDU
  - [x] Error Response PDU

### 2.3 MIB Implementation
- [x] Core MIB Infrastructure
  - [x] Create OID tree structure
  - [x] Implement node traversal
  - [x] Add value storage system
- [x] System Group (.1.3.6.1.2.1.1)
  - [x] Basic system OIDs (sysDescr, sysObjectID)
  - [x] Time-based OIDs (sysUpTime)
  - [x] Configurable OIDs (sysContact, sysName, sysLocation)
- [x] Private Group (.1.3.6.1.4.1.63050)
  - [x] Power monitoring OIDs
  - [x] Configuration OIDs
  - [x] Statistics OIDs

### 2.4 Protocol Operations
- [x] Message Processing
  - [x] Implement message decoder
  - [x] Add message validator
  - [x] Create response builder
- [x] Request Handlers
  - [x] GetRequest processor
  - [x] GetNextRequest processor
  - [x] Error handling system
- [x] Security & Performance
  - [x] Community string validation
  - [x] Request rate limiting
  - [x] Response time optimization

### 2.5 Testing Framework
- [x] Framework Setup
  - [x] Add Unity test framework to platformio.ini
  - [x] Create test environments (unit, integration, performance)
  - [x] Configure test runners for each category
  - [x] Set up test directory structure

- [x] Unit Tests
  - [x] ASN.1/BER Encoding/Decoding
    - [x] Test primitive types (INTEGER, OCTET STRING, NULL, OID)
    - [x] Test constructed types (SEQUENCE, SEQUENCE OF)
    - [x] Test boundary conditions
    - [x] Test error cases
  - [x] Message Structure
    - [x] Test PDU construction/parsing
    - [x] Test version handling
    - [x] Test community string processing
    - [x] Test request/response formats
  - [x] MIB Operations
    - [x] Test node traversal
    - [x] Test value storage/retrieval
    - [x] Test OID validation
    - [x] Test data type handling

- [x] Integration Tests
  - [x] End-to-end Request Handling
    - [x] Test complete GetRequest flow
    - [x] Test complete GetNextRequest flow
    - [x] Test response generation
    - [x] Test error handling
  - [x] MIB Walk Operations
    - [x] Test sequential OID retrieval
    - [x] Test boundary conditions
    - [x] Test with various MIB sizes
    - [x] Test traversal performance
  - [x] Error Conditions
    - [x] Test malformed packets
    - [x] Test invalid community strings
    - [x] Test rate limiting behavior
    - [x] Test recovery mechanisms

  - [x] Performance Tests
  - [x] Response Timing
    - [x] Measure request-to-response latency
    - [x] Test under various load conditions
    - [x] Verify 100ms response requirement
    - [x] Profile critical paths
  - [x] Memory Usage
    - [x] Track heap allocation
    - [x] Monitor stack usage
    - [x] Verify RAM usage under 50%
    - [x] Test memory recovery
  - [x] Rate Limiting
    - [x] Verify request throttling
    - [x] Test concurrent request handling
    - [x] Measure system resource impact
    - [x] Test sustained load behavior

  - [x] Mock Components
  - [x] Hardware Abstraction
    - [x] Create W5500 network interface mock
    - [x] Implement GPIO simulation
    - [x] Set up flash memory simulation
  - [x] Test Utilities
    - [x] Create packet generators
    - [x] Implement timing measurement tools
    - [x] Add memory tracking utilities

## 3. Power Monitoring System
- [x] Set up GPIO27 for power detection
  - [x] Configure pin as input with proper pull-up/down
  - [x] Implement interrupt handling for state changes
  - [x] Add debouncing for reliable detection
- [x] Create power status monitoring
  - [x] Implement status checking function
  - [x] Add status to MIB structure
  - [x] Create status change notification system

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
- [x] Create hardware test suite
  - [x] Test power monitoring circuit
  - [x] Validate GPIO configurations
  - [x] Test W5500 communication
  - [x] Verify factory reset functionality
- [x] Implement SNMP testing
  - [x] Test GetRequest operations
  - [x] Validate GetNextRequest functionality
  - [x] Test community string validation
  - [x] Verify MIB walk operations
- [x] Performance testing
  - [x] Measure SNMP response times
  - [x] Verify CLI responsiveness
  - [x] Test power state detection speed
  - [x] Validate factory reset timing

## 7. Documentation
- [x] Code documentation
  - [x] Add function documentation
  - [x] Document MIB structure
  - [x] Add hardware connection details
  - [x] Document configuration options
- [x] User documentation
  - [x] Create CLI command guide
  - [x] Add network configuration guide
  - [x] Provide SNMP usage examples
  - [x] Create troubleshooting guide

## 8. Safety & Security Implementation
- [x] Implement security measures
  - [x] Add request rate limiting
  - [x] Implement access logging
  - [x] Add buffer overflow protection
  - [x] Implement packet validation
- [x] Add circuit protection
  - [x] Implement GPIO protection
  - [x] Add power monitoring isolation
  - [x] Verify voltage levels

## 9. Build System & Quality Control
- [x] Set up PlatformIO configuration
  - [x] Configure build environment
  - [x] Set up dependency management
  - [x] Add build documentation
- [x] Implement error handling
  - [x] Add global error handling system
  - [x] Implement recovery procedures
  - [x] Add error logging

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
