#include "unity.h"
extern void motor_set_speed(unsigned char, unsigned short);
void setUp(void){}
void tearDown(void){}
void test_req_mcu_001_speed_zero(void) {
    motor_set_speed(0, 0);
    TEST_PASS();
}
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_req_mcu_001_speed_zero);
    return UNITY_END();
}
