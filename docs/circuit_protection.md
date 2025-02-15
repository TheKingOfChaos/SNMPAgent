# Circuit Protection Documentation

## Overview
The circuit protection system provides hardware-level safety features for GPIO pins, including voltage monitoring, current limiting, and fault detection. It is designed to protect both the RP2040 microcontroller and connected external devices.

## Hardware Protection Features

### 1. Input Protection Circuit
```
External Input -> 10kΩ -> GPIO Pin
                    |
                 100kΩ
                    |
                   GND
```
- Voltage divider provides 1:11 ratio
- Maximum input voltage: 36V (3.3V at GPIO)
- Built-in current limiting through resistors
- Protection against reverse voltage

### 2. Output Protection Circuit
```
GPIO Pin -> 330Ω -> Output
              |
           Schottky
              |
             GND
```
- Current limiting resistor
- Flyback diode for inductive loads
- PWM current control capability
- Short circuit protection

### 3. Isolated Input Circuit
```
External Input -> Optocoupler -> 10kΩ -> GPIO Pin
                                   |
                                  GND
```
- Complete galvanic isolation
- 2.5kV isolation voltage
- High noise immunity
- Suitable for high-voltage inputs

## Software Implementation

### Protection Configuration
```cpp
CircuitProtection::ProtectionConfig config = {
    .type = CircuitProtection::ProtectionType::INPUT_WITH_PULLUP,
    .interruptMode = CircuitProtection::InterruptMode::FALLING,
    .maxVoltage = 3.3f,
    .currentLimit = 20,  // 20mA
    .maxTriggers = 5,
    .triggerWindow = 1000  // ms
};

protection.protectPin(27, config);
```

### Protection Types
1. INPUT_WITH_PULLUP
   - Internal pull-up enabled
   - Voltage monitoring
   - Interrupt capability
   - Suitable for switches/sensors

2. INPUT_WITH_PULLDOWN
   - External pull-down required
   - Voltage monitoring
   - Interrupt capability
   - Suitable for active-high signals

3. OUTPUT_WITH_CURRENT_LIMIT
   - PWM-based current limiting
   - Short circuit detection
   - Voltage monitoring
   - Suitable for driving loads

4. ISOLATED_INPUT
   - For optocoupler inputs
   - Noise filtering
   - Debounce support
   - Suitable for high-voltage isolation

## Fault Detection & Handling

### 1. Voltage Monitoring
- Continuous ADC sampling
- Configurable thresholds
- Automatic shutdown on overvoltage
- Status reporting via MIB

### 2. Current Limiting
- PWM-based limiting for outputs
- Hardware current sense
- Automatic shutdown on overcurrent
- Configurable limits

### 3. Fault Response
```cpp
protection.setFaultCallback([](uint8_t pin) {
    // Disable the faulted pin
    // Log the fault
    // Update MIB status
    // Notify operator
});
```

### 4. Recovery Procedures
1. Automatic Recovery
   - Cooldown period
   - Gradual re-enabling
   - Fault counter reset

2. Manual Recovery
   ```cpp
   // Reset fault condition
   protection.resetTriggerCount(pin);
   
   // Re-enable protection
   protection.protectPin(pin, config);
   ```

## Integration with SNMP

### MIB Structure
1. Protection Status
   - OID: .1.3.6.1.4.1.63050.3.1.x
   - Per-pin status information
   - Fault counters
   - Current state

2. Configuration
   - OID: .1.3.6.1.4.1.63050.3.2.x
   - Protection settings
   - Threshold values
   - Recovery parameters

### SNMP Commands
1. Get protection status:
   ```bash
   snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.3.1.27.0
   ```

2. Get fault count:
   ```bash
   snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.3.1.27.1
   ```

## Testing & Validation

### 1. Hardware Tests
- Overvoltage protection
- Current limiting
- Isolation verification
- Response time measurement

### 2. Software Tests
```cpp
// Test voltage monitoring
TEST_ASSERT_TRUE(protection.checkVoltage(27));

// Test fault detection
TEST_ASSERT_TRUE(faultDetected);
TEST_ASSERT_EQUAL(27, faultPin);
```

### 3. Performance Metrics
- Response Time: < 100μs
- Voltage Accuracy: ±1%
- Current Limit Accuracy: ±5%
- Maximum Protected Pins: 16

## Troubleshooting Guide

### Common Issues

1. False Triggers
   - Check debounce settings
   - Verify voltage thresholds
   - Inspect for noise sources
   ```cpp
   // Increase debounce time
   config.triggerWindow = 100;  // 100ms
   ```

2. Current Limiting Issues
   - Verify load requirements
   - Check PWM frequency
   - Measure actual current
   ```cpp
   // Adjust current limit
   config.currentLimit = 10;  // 10mA
   ```

3. Recovery Problems
   - Check fault conditions
   - Verify timing parameters
   - Reset protection if needed
   ```cpp
   protection.unprotectPin(pin);
   protection.protectPin(pin, config);
   ```

### Diagnostic Commands
```cpp
// Get pin status
auto status = protection.getPinStatus(pin);
printf("Enabled: %d\n", status.enabled);
printf("Type: %d\n", status.type);
printf("Triggers: %lu\n", status.triggerCount);

// Check voltage
bool voltageOK = protection.checkVoltage(pin);
```

## Safety Guidelines

1. Hardware Design
   - Use proper voltage dividers
   - Include current limiting
   - Add reverse protection
   - Maintain isolation barriers

2. Software Configuration
   - Set conservative limits
   - Enable fault detection
   - Implement recovery logic
   - Log all events

3. Maintenance
   - Regular voltage checks
   - Fault counter monitoring
   - Protection circuit testing
   - Component inspection
