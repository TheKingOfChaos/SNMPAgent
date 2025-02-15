#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include "MIB.h"
#include "ASN1Types.h"
#include <cstdint>

class PowerMonitor {
public:
    explicit PowerMonitor(MIB& mib);
    
    void begin();
    bool isPowerPresent() const;
    uint32_t getPowerLossCount() const;
    uint32_t getLastPowerLossTime() const;
    
private:
    static constexpr uint8_t POWER_PIN = 27;  // GPIO27 for power monitoring
    static constexpr uint32_t DEBOUNCE_TIME = 50;  // 50ms debounce
    
    // Power state values
    static constexpr uint8_t POWER_STATE_ON = 1;
    static constexpr uint8_t POWER_STATE_OFF = 0;
    
    // Power monitoring OIDs
    static constexpr char POWER_STATE_OID[] = "1.3.6.1.4.1.63050.1.1.0";     // powerState.0
    static constexpr char LAST_POWER_LOSS_OID[] = "1.3.6.1.4.1.63050.1.2.0"; // lastPowerLoss.0
    static constexpr char POWER_LOSS_COUNT_OID[] = "1.3.6.1.4.1.63050.1.3.0"; // powerLossCount.0
    
    MIB& mib_;
    unsigned long lastInterruptTime_;
    
    void handleInterrupt();
    void initializeMIBNodes();
};

#endif // POWER_MONITOR_H
