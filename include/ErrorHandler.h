#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>
#include <functional>

class ErrorHandler {
public:
    // Error severity levels
    enum class Severity {
        INFO,       // Informational messages
        WARNING,    // Non-critical issues
        ERROR,      // Recoverable errors
        CRITICAL    // System-halting errors
    };
    
    // Error categories
    enum class Category {
        NETWORK,    // Network/communication errors
        HARDWARE,   // Hardware-related issues
        MEMORY,     // Memory allocation/usage issues
        SECURITY,   // Security violations
        SYSTEM,     // System-level errors
        PROTOCOL    // Protocol-specific errors
    };
    
    // Error information structure
    struct ErrorInfo {
        Severity severity;
        Category category;
        uint32_t code;
        char message[64];  // Fixed size buffer for message
        unsigned long timestamp;
    };
    
    // Error callback type
    using ErrorCallback = void (*)(const ErrorInfo&);
    
    // Singleton instance
    static ErrorHandler& getInstance() {
        static ErrorHandler instance;
        return instance;
    }
    
    // Error reporting
    void reportError(Severity severity, Category category, uint32_t code, const char* message);
    
    // Error handling registration
    bool registerCallback(ErrorCallback callback);
    void removeCallback(ErrorCallback callback);
    
    // Recovery functions
    void attemptRecovery(Category category);
    void resetErrors(Category category);
    
    // Error status
    bool hasErrors(Category category) const;
    bool hasCriticalErrors() const;
    size_t getErrors(Category category, ErrorInfo* buffer, size_t bufferSize) const;
    
    // System status
    bool isSystemHealthy() const;
    void clearAllErrors();
    
private:
    ErrorHandler() : error_count_(0), callback_count_(0) {}  // Private constructor for singleton
    
    // Error storage
    static constexpr size_t MAX_ERRORS = 100;
    static constexpr size_t MAX_CALLBACKS = 10;
    
    ErrorInfo errors_[MAX_ERRORS];
    ErrorCallback callbacks_[MAX_CALLBACKS];
    size_t error_count_;
    size_t callback_count_;
    
    // Recovery procedures
    void handleNetworkError(const ErrorInfo& error);
    void handleHardwareError(const ErrorInfo& error);
    void handleMemoryError(const ErrorInfo& error);
    void handleSecurityError(const ErrorInfo& error);
    void handleSystemError(const ErrorInfo& error);
    void handleProtocolError(const ErrorInfo& error);
    
    // Error management
    void pruneErrors();
    void notifyCallbacks(const ErrorInfo& error);
    bool isRecoverable(const ErrorInfo& error) const;
};

// Global error reporting macros
#define REPORT_ERROR(sev, cat, code, msg) \
    ErrorHandler::getInstance().reportError(sev, cat, code, msg)

#define REPORT_INFO(cat, code, msg) \
    REPORT_ERROR(ErrorHandler::Severity::INFO, cat, code, msg)

#define REPORT_WARNING(cat, code, msg) \
    REPORT_ERROR(ErrorHandler::Severity::WARNING, cat, code, msg)

#define REPORT_ERROR_CRITICAL(cat, code, msg) \
    REPORT_ERROR(ErrorHandler::Severity::CRITICAL, cat, code, msg)

// Error checking macros
#define CHECK_MEMORY(ptr) \
    if (!ptr) { \
        REPORT_ERROR_CRITICAL(ErrorHandler::Category::MEMORY, 0x1001, "Memory allocation failed"); \
        return false; \
    }

#define CHECK_BOUNDS(val, min, max) \
    if (val < min || val > max) { \
        REPORT_ERROR(ErrorHandler::Severity::ERROR, ErrorHandler::Category::SYSTEM, 0x2001, \
                    "Value out of bounds"); \
        return false; \
    }

#endif // ERROR_HANDLER_H
