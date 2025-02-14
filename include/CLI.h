#ifndef CLI_H
#define CLI_H

#include "SerialCom.h"
#include "Settings.h" // For SettingsManager
#include <Arduino.h>

class CLI {
public:
    static constexpr size_t MAX_COMMAND_LENGTH = 64;
    static constexpr size_t MAX_ARGS = 8;
    static constexpr char COMMAND_DELIMITER = ' ';

    CLI(SerialCom& serial, SettingsManager& settings);
    ~CLI() = default;

    // Process CLI input
    void process();

    // Command handlers
    void handleHelp();
    void handleSetCommunity(const char* community);
    void handleSetNetwork(int argc, char* argv[]);
    void handleStatus();
    void handleFactoryReset();

private:
    SerialCom& serialCom;
    SettingsManager& settings;
    char commandBuffer[MAX_COMMAND_LENGTH];
    size_t bufferIndex;

    // Command parsing
    void parseCommand();
    bool splitArgs(char* input, char* argv[], int& argc);
    void clearBuffer();
    
    // Input validation
    bool isValidCommunity(const char* community);
    bool isValidIPAddress(const char* ip);
    
    // Utility functions
    void printError(const char* message);
    void printSuccess(const char* message);
    
    // Help messages
    void printCommandHelp(const char* command, const char* usage, const char* description);

    // Prevent copying
    CLI(const CLI&) = delete;
    CLI& operator=(const CLI&) = delete;
};

#endif // CLI_H
