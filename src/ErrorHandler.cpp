#include "ErrorHandler.h"
#include <string.h>

void ErrorHandler::reportError(Severity severity, Category category, uint32_t code, const char* message) {
    // Create error info
    ErrorInfo error;
    error.severity = severity;
    error.category = category;
    error.code = code;
    strncpy(error.message, message, sizeof(error.message) - 1);
    error.message[sizeof(error.message) - 1] = '\0';
    error.timestamp = millis();
    
    // Add to error array
    if (error_count_ < MAX_ERRORS) {
        errors_[error_count_++] = error;
    } else {
        pruneErrors();
        errors_[MAX_ERRORS - 1] = error;
    }
    
    // Notify callbacks
    notifyCallbacks(error);
    
    // Handle critical errors immediately
    if (severity == Severity::CRITICAL) {
        attemptRecovery(category);
    }
}

bool ErrorHandler::registerCallback(ErrorCallback callback) {
    if (callback_count_ < MAX_CALLBACKS) {
        callbacks_[callback_count_++] = callback;
        return true;
    }
    return false;
}

void ErrorHandler::removeCallback(ErrorCallback callback) {
    for (size_t i = 0; i < callback_count_; i++) {
        if (callbacks_[i] == callback) {
            // Shift remaining callbacks
            for (size_t j = i; j < callback_count_ - 1; j++) {
                callbacks_[j] = callbacks_[j + 1];
            }
            callback_count_--;
            break;
        }
    }
}

void ErrorHandler::attemptRecovery(Category category) {
    // Find most recent error for this category
    for (int i = error_count_ - 1; i >= 0; i--) {
        if (errors_[i].category == category) {
            // Call appropriate recovery handler
            switch (category) {
                case Category::NETWORK:
                    handleNetworkError(errors_[i]);
                    break;
                case Category::HARDWARE:
                    handleHardwareError(errors_[i]);
                    break;
                case Category::MEMORY:
                    handleMemoryError(errors_[i]);
                    break;
                case Category::SECURITY:
                    handleSecurityError(errors_[i]);
                    break;
                case Category::SYSTEM:
                    handleSystemError(errors_[i]);
                    break;
                case Category::PROTOCOL:
                    handleProtocolError(errors_[i]);
                    break;
            }
            break;
        }
    }
}

void ErrorHandler::resetErrors(Category category) {
    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < error_count_; read_idx++) {
        if (errors_[read_idx].category != category) {
            if (write_idx != read_idx) {
                errors_[write_idx] = errors_[read_idx];
            }
            write_idx++;
        }
    }
    error_count_ = write_idx;
}

bool ErrorHandler::hasErrors(Category category) const {
    for (size_t i = 0; i < error_count_; i++) {
        if (errors_[i].category == category) {
            return true;
        }
    }
    return false;
}

bool ErrorHandler::hasCriticalErrors() const {
    for (size_t i = 0; i < error_count_; i++) {
        if (errors_[i].severity == Severity::CRITICAL) {
            return true;
        }
    }
    return false;
}

size_t ErrorHandler::getErrors(Category category, ErrorInfo* buffer, size_t bufferSize) const {
    size_t count = 0;
    for (size_t i = 0; i < error_count_ && count < bufferSize; i++) {
        if (errors_[i].category == category) {
            buffer[count++] = errors_[i];
        }
    }
    return count;
}

bool ErrorHandler::isSystemHealthy() const {
    return !hasCriticalErrors() && error_count_ < MAX_ERRORS / 2;
}

void ErrorHandler::clearAllErrors() {
    error_count_ = 0;
}

void ErrorHandler::pruneErrors() {
    // Keep only the most recent errors
    if (error_count_ >= MAX_ERRORS) {
        size_t toKeep = MAX_ERRORS * 3 / 4;  // Keep 75% of max
        size_t toRemove = error_count_ - toKeep;
        
        // Shift remaining errors to the beginning
        for (size_t i = 0; i < toKeep; i++) {
            errors_[i] = errors_[i + toRemove];
        }
        error_count_ = toKeep;
    }
}

void ErrorHandler::notifyCallbacks(const ErrorInfo& error) {
    for (size_t i = 0; i < callback_count_; i++) {
        if (callbacks_[i]) {
            callbacks_[i](error);
        }
    }
}

bool ErrorHandler::isRecoverable(const ErrorInfo& error) const {
    return error.severity != Severity::CRITICAL;
}

// Recovery procedures
void ErrorHandler::handleNetworkError(const ErrorInfo& error) {
    if (!isRecoverable(error)) {
        // Reset network interface
        // Reinitialize W5500
        // Attempt DHCP renewal
    }
}

void ErrorHandler::handleHardwareError(const ErrorInfo& error) {
    if (!isRecoverable(error)) {
        // Reset affected hardware
        // Reconfigure GPIO
        // Check voltage levels
    }
}

void ErrorHandler::handleMemoryError(const ErrorInfo& error) {
    if (!isRecoverable(error)) {
        // Free non-essential memory
        // Reset memory pools
        // Clear caches
    }
}

void ErrorHandler::handleSecurityError(const ErrorInfo& error) {
    if (!isRecoverable(error)) {
        // Block offending IP
        // Reset security counters
        // Clear sensitive data
    }
}

void ErrorHandler::handleSystemError(const ErrorInfo& error) {
    if (!isRecoverable(error)) {
        // Reset system state
        // Reload configuration
        // Restart services
    }
}

void ErrorHandler::handleProtocolError(const ErrorInfo& error) {
    if (!isRecoverable(error)) {
        // Reset protocol state
        // Clear message queues
        // Reinitialize handlers
    }
}
