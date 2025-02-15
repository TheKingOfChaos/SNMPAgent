# Power Monitoring System Documentation

## Overview
The power monitoring system is responsible for detecting and reporting mains power status through SNMP. It uses GPIO27 on the Raspberry Pi Pico to monitor power state and maintains statistics about power losses in the MIB.

## Hardware Configuration

### GPIO Pin Assignment
- **GPIO27**: Power monitoring input
  - Configured as INPUT_PULLUP
  - HIGH = Power present
  - LOW = Power loss
  - Internal pull-up resistor enabled for reliable state detection

### Circuit Protection Requirements
1. Voltage Divider
   ```
   Mains Detection Circuit -> 10kΩ -> GPIO27
                                 |
                             100kΩ
                                 |
                               GND
   ```
2. Optocoupler Isolation
   - Required for galvanic isolation from mains circuit
   - Recommended part: PC817 or equivalent
   - Maximum CTR (Current Transfer Ratio): 50%
   - Input current: 5mA @ 5V

## Software Implementation

### MIB Structure
The power monitoring system exposes three OIDs under the private enterprise number (63050):

1. Power State (1.3.6.1.4.1.63050.1.1.0)
   - Type: INTEGER
   - Access: READ-ONLY
   - Values:
     * 1 = Power Present
     * 0 = Power Loss

2. Last Power Loss Time (1.3.6.1.4.1.63050.1.2.0)
   - Type: INTEGER
   - Access: READ-ONLY
   - Value: Timestamp in milliseconds since boot

3. Power Loss Count (1.3.6.1.4.1.63050.1.3.0)
   - Type: INTEGER
   - Access: READ-ONLY
   - Value: Number of power losses detected

### Debounce Implementation
- 50ms debounce period between state changes
- Implemented in hardware interrupt handler
- Prevents false triggers from noise or transients

### Class Interface
```cpp
class PowerMonitor {
public:
    explicit PowerMonitor(MIB& mib);
    void begin();  // Initialize GPIO and interrupts
    bool isPowerPresent() const;  // Get current power state
    uint32_t getPowerLossCount() const;  // Get total power losses
    uint32_t getLastPowerLossTime() const;  // Get last loss timestamp
};
```

## Usage Example

### Initialization
```cpp
MIB mib;
PowerMonitor powerMonitor(mib);

void setup() {
    mib.initialize();
    powerMonitor.begin();
}
```

### SNMP Queries
1. Get current power state:
```bash
snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.1.1.0
```

2. Get last power loss time:
```bash
snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.1.2.0
```

3. Get power loss count:
```bash
snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.1.3.0
```

## Performance Characteristics

### Response Times
- Interrupt Response: < 10ms
- State Change Detection: < 60ms (including debounce)
- SNMP Query Response: < 100ms

### Memory Usage
- Static RAM: ~100 bytes
- Dynamic RAM: ~32 bytes per MIB value
- Flash Usage: ~2KB for code

## Error Handling

### Hardware Faults
1. Input Pin Stuck
   - Detected by periodic pin state verification
   - Reported through MIB error status

2. Noise/Interference
   - Mitigated by debounce implementation
   - Hardware filtering through RC circuit

### Software Protection
1. Memory Management
   - Proper cleanup of ASN.1 objects
   - Smart pointers for MIB values

2. Interrupt Safety
   - Critical sections protected
   - Atomic operations for counters

## Testing

### Unit Tests
Located in `test/integration/test_power_monitoring.cpp`:
1. Pin Configuration Test
2. Interrupt Handling Test
3. Power Loss Detection Test
4. Debounce Behavior Test

### Integration Tests
1. End-to-end SNMP query tests
2. Power state change notification tests
3. Long-term stability tests

## Troubleshooting Guide

### Common Issues

1. False Power Loss Detection
   - Check voltage divider values
   - Verify debounce timing
   - Inspect for EMI sources

2. Missing State Changes
   - Verify interrupt configuration
   - Check pull-up resistor
   - Test optocoupler operation

3. Incorrect Timestamps
   - Verify system time configuration
   - Check for overflow handling

### Diagnostic Commands
```cpp
// Get raw pin state
bool rawState = digitalRead(27);

// Get debounced state
bool powerState = powerMonitor.isPowerPresent();

// Get statistics
uint32_t lossCount = powerMonitor.getPowerLossCount();
uint32_t lastLoss = powerMonitor.getLastPowerLossTime();
```
