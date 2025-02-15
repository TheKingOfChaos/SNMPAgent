# SNMP Pico Power Monitor User Guide

## Introduction
The SNMP Pico Power Monitor is a network-connected device that monitors mains power status and provides real-time updates through SNMP. This guide covers installation, configuration, and usage of the device.

## Installation

### Hardware Setup
1. Mount the device in a suitable enclosure near the power source to be monitored
2. Connect the power monitoring circuit:
   ```
   AC Power Source -> Optocoupler -> Voltage Divider -> GPIO27
   ```
3. Connect network cable to the W5500 Ethernet module
4. Power the device using a 5V USB power supply

### Network Configuration
1. Connect to the device via USB serial port:
   ```bash
   # Linux/macOS
   screen /dev/ttyUSB0 115200
   
   # Windows
   # Use PuTTY with COM port at 115200 baud
   ```

2. Configure network settings using CLI:
   ```
   > config network
   DHCP (y/n)? n
   IP Address: 192.168.1.100
   Subnet Mask: 255.255.255.0
   Gateway: 192.168.1.1
   Save? y
   ```

3. Configure SNMP settings:
   ```
   > config snmp
   Community String: public
   Save? y
   ```

## Monitoring Power Status

### Using SNMP Commands

1. Get current power state:
   ```bash
   snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.1.1.0
   # Returns:
   # INTEGER: 1 (Power Present)
   # or
   # INTEGER: 0 (Power Loss)
   ```

2. Get last power loss time:
   ```bash
   snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.1.2.0
   # Returns:
   # INTEGER: <timestamp in milliseconds since boot>
   ```

3. Get power loss count:
   ```bash
   snmpget -v1 -c public 192.168.1.100 1.3.6.1.4.1.63050.1.3.0
   # Returns:
   # INTEGER: <number of power losses detected>
   ```

### Using CLI Interface

1. View current status:
   ```
   > status
   Power State: Present
   Last Power Loss: 2h 15m ago
   Total Power Losses: 3
   ```

2. View detailed statistics:
   ```
   > stats power
   Power Loss Events:
   1. 2024-02-14 15:30:45 (Duration: 5m 20s)
   2. 2024-02-14 18:22:10 (Duration: 2m 15s)
   3. 2024-02-15 01:45:30 (Duration: 3m 45s)
   ```

## Integration with Monitoring Systems

### Nagios Configuration
```cfg
define service {
    host_name           power-monitor
    service_description Power Status
    check_command       check_snmp!-C public -o 1.3.6.1.4.1.63050.1.1.0 -w 1:1
    check_interval      1
}
```

### Zabbix Template
```yaml
items:
  - name: Power State
    key: snmp.powerState
    oid: 1.3.6.1.4.1.63050.1.1.0
    type: SNMP agent
    value_type: NUMERIC
    
triggers:
  - name: Power Loss Detected
    expression: {power-monitor:snmp.powerState.last()}=0
    priority: HIGH
```

## Troubleshooting

### Common Issues

1. Device Not Responding
   - Check network cable connection
   - Verify IP configuration
   - Ensure power supply is stable

2. False Power Loss Alerts
   - Check power monitoring circuit connections
   - Verify voltage divider values
   - Adjust debounce time if needed:
     ```
     > config power
     Debounce Time (ms): 75
     Save? y
     ```

3. Network Connectivity Issues
   - Use ping to verify basic connectivity:
     ```bash
     ping 192.168.1.100
     ```
   - Check network settings:
     ```
     > show network
     IP: 192.168.1.100
     Mask: 255.255.255.0
     Gateway: 192.168.1.1
     DHCP: Disabled
     ```

### LED Status Indicators

1. Power LED (Green)
   - Solid: Device powered and operational
   - Off: Device unpowered
   - Blinking: Factory reset in progress

2. Network LED (Yellow)
   - Solid: Network connected
   - Blinking: Network activity
   - Off: No network connection

3. Status LED (Blue)
   - Solid: Mains power present
   - Off: Mains power loss
   - Blinking: Error condition

### Factory Reset
If needed, you can reset the device to default settings:
1. Press and hold the reset button (GPIO22)
2. Wait for power LED to start blinking (about 10 seconds)
3. Release button
4. Device will reset with default configuration:
   - DHCP enabled
   - Community string: "public"
   - Debounce time: 50ms

## Maintenance

### Firmware Updates
1. Download latest firmware from repository
2. Connect via USB
3. Enter bootloader mode:
   ```
   > system update
   Ready for firmware update...
   ```
4. Use provided update tool:
   ```bash
   python update_tool.py -p /dev/ttyUSB0 -f firmware.bin
   ```

### Backup Configuration
Save current configuration to file:
```
> config save backup.cfg
Configuration saved successfully
```

Restore from backup:
```
> config load backup.cfg
Configuration restored successfully
```

## Technical Support
- GitHub Issues: https://github.com/org/snmp-pico-power/issues
- Email Support: support@example.com
- Documentation: https://docs.example.com/snmp-pico-power
