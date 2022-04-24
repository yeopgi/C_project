#include <cstdio>

uint8_t get_id()
{
    // 특정 레지스터에 접근해 디바이스의 아이디 리턴 받는다.
}

int main(void)
{
    uint16_t device_id;
    uint8_t MSB_device_id = get_id();
    uint8_t LSB_device_id = get_id();
    device_id = uint16((MSB_device_id << 8) + LSB_device_id);

    return 0;
}

