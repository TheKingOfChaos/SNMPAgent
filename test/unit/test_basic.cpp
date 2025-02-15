#include <unity.h>

void test_basic_assertion(void) {
    TEST_ASSERT_EQUAL(1, 1);
}

#ifndef TEST_NATIVE
// Arduino-specific code
#include <Arduino.h>

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_basic_assertion);
    UNITY_END();
}

void loop() {
    delay(100);
}

#else
// Native test code
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_basic_assertion);
    return UNITY_END();
}
#endif
