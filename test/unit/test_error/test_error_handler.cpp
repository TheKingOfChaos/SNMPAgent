#include <unity.h>
#include "ErrorHandler.h"
#include <vector>

// Test callback data
static std::vector<ErrorHandler::ErrorInfo> callbackErrors;
static void testCallback(const ErrorHandler::ErrorInfo& error) {
    callbackErrors.push_back(error);
}

void setUp(void) {
    ErrorHandler::getInstance().clearAllErrors();
    callbackErrors.clear();
    ErrorHandler::getInstance().registerCallback(testCallback);
}

void tearDown(void) {
    ErrorHandler::getInstance().removeCallback(testCallback);
}

void test_error_reporting() {
    // Report various types of errors
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::WARNING,
        ErrorHandler::Category::NETWORK,
        0x1001,
        "Network timeout"
    );
    
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::ERROR,
        ErrorHandler::Category::MEMORY,
        0x2001,
        "Memory allocation failed"
    );
    
    // Verify errors were recorded
    TEST_ASSERT_TRUE(ErrorHandler::getInstance().hasErrors(ErrorHandler::Category::NETWORK));
    TEST_ASSERT_TRUE(ErrorHandler::getInstance().hasErrors(ErrorHandler::Category::MEMORY));
    
    // Verify callback was called
    TEST_ASSERT_EQUAL(2, callbackErrors.size());
    TEST_ASSERT_EQUAL(ErrorHandler::Category::NETWORK, callbackErrors[0].category);
    TEST_ASSERT_EQUAL(ErrorHandler::Category::MEMORY, callbackErrors[1].category);
}

void test_critical_error_handling() {
    // Report a critical error
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::CRITICAL,
        ErrorHandler::Category::HARDWARE,
        0x3001,
        "Hardware failure"
    );
    
    // Verify critical error state
    TEST_ASSERT_TRUE(ErrorHandler::getInstance().hasCriticalErrors());
    TEST_ASSERT_FALSE(ErrorHandler::getInstance().isSystemHealthy());
    
    // Verify callback was called
    TEST_ASSERT_EQUAL(1, callbackErrors.size());
    TEST_ASSERT_EQUAL(ErrorHandler::Severity::CRITICAL, callbackErrors[0].severity);
}

void test_error_categories() {
    // Report errors in different categories
    const std::vector<ErrorHandler::Category> categories = {
        ErrorHandler::Category::NETWORK,
        ErrorHandler::Category::HARDWARE,
        ErrorHandler::Category::MEMORY,
        ErrorHandler::Category::SECURITY,
        ErrorHandler::Category::SYSTEM,
        ErrorHandler::Category::PROTOCOL
    };
    
    for (const auto& category : categories) {
        ErrorHandler::getInstance().reportError(
            ErrorHandler::Severity::WARNING,
            category,
            0x1000,
            "Test error"
        );
    }
    
    // Verify each category
    for (const auto& category : categories) {
        TEST_ASSERT_TRUE(ErrorHandler::getInstance().hasErrors(category));
        auto errors = ErrorHandler::getInstance().getErrors(category);
        TEST_ASSERT_EQUAL(1, errors.size());
        TEST_ASSERT_EQUAL(category, errors[0].category);
    }
}

void test_error_pruning() {
    // Fill error buffer beyond capacity
    for (size_t i = 0; i < 150; i++) {  // MAX_ERRORS is 100
        ErrorHandler::getInstance().reportError(
            ErrorHandler::Severity::INFO,
            ErrorHandler::Category::SYSTEM,
            0x1000 + i,
            "Test error"
        );
    }
    
    // Verify error count is within limits
    auto errors = ErrorHandler::getInstance().getErrors(ErrorHandler::Category::SYSTEM);
    TEST_ASSERT_LESS_OR_EQUAL(100, errors.size());
    
    // Verify oldest errors were removed
    TEST_ASSERT_GREATER_THAN(0x1000 + 25, errors[0].code);
}

void test_error_recovery() {
    // Report recoverable error
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::ERROR,
        ErrorHandler::Category::NETWORK,
        0x1001,
        "Network error"
    );
    
    // Attempt recovery
    ErrorHandler::getInstance().attemptRecovery(ErrorHandler::Category::NETWORK);
    
    // Report unrecoverable error
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::CRITICAL,
        ErrorHandler::Category::HARDWARE,
        0x2001,
        "Hardware failure"
    );
    
    // Verify system health
    TEST_ASSERT_FALSE(ErrorHandler::getInstance().isSystemHealthy());
}

void test_error_clearing() {
    // Add errors in multiple categories
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::WARNING,
        ErrorHandler::Category::NETWORK,
        0x1001,
        "Network warning"
    );
    
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::ERROR,
        ErrorHandler::Category::MEMORY,
        0x2001,
        "Memory error"
    );
    
    // Clear specific category
    ErrorHandler::getInstance().resetErrors(ErrorHandler::Category::NETWORK);
    TEST_ASSERT_FALSE(ErrorHandler::getInstance().hasErrors(ErrorHandler::Category::NETWORK));
    TEST_ASSERT_TRUE(ErrorHandler::getInstance().hasErrors(ErrorHandler::Category::MEMORY));
    
    // Clear all errors
    ErrorHandler::getInstance().clearAllErrors();
    TEST_ASSERT_FALSE(ErrorHandler::getInstance().hasErrors(ErrorHandler::Category::MEMORY));
    TEST_ASSERT_TRUE(ErrorHandler::getInstance().isSystemHealthy());
}

void test_callback_management() {
    // Create test callbacks
    int count1 = 0, count2 = 0;
    auto callback1 = [&count1](const ErrorHandler::ErrorInfo&) { count1++; };
    auto callback2 = [&count2](const ErrorHandler::ErrorInfo&) { count2++; };
    
    // Register callbacks
    ErrorHandler::getInstance().registerCallback(callback1);
    ErrorHandler::getInstance().registerCallback(callback2);
    
    // Generate error
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::WARNING,
        ErrorHandler::Category::SYSTEM,
        0x1001,
        "Test error"
    );
    
    // Verify both callbacks were called
    TEST_ASSERT_EQUAL(1, count1);
    TEST_ASSERT_EQUAL(1, count2);
    
    // Remove one callback
    ErrorHandler::getInstance().removeCallback(callback1);
    
    // Generate another error
    ErrorHandler::getInstance().reportError(
        ErrorHandler::Severity::WARNING,
        ErrorHandler::Category::SYSTEM,
        0x1002,
        "Test error 2"
    );
    
    // Verify only remaining callback was called
    TEST_ASSERT_EQUAL(1, count1);
    TEST_ASSERT_EQUAL(2, count2);
}

void setup() {
    delay(2000); // Allow board to settle
    UNITY_BEGIN();
    
    RUN_TEST(test_error_reporting);
    RUN_TEST(test_critical_error_handling);
    RUN_TEST(test_error_categories);
    RUN_TEST(test_error_pruning);
    RUN_TEST(test_error_recovery);
    RUN_TEST(test_error_clearing);
    RUN_TEST(test_callback_management);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}
