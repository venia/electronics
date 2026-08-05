#include <MyButtonController.hpp>
#include <main.h>
#include <touchgfx/hal/HAL.hpp>

void MyButtonController::init()
{
    previousState = 0x00;
}

bool MyButtonController::sample(uint8_t& key)
{
    // Проверяем физическую кнопку K1 (PC13), активный уровень HIGH (подтверждено экспериментально)
    if ((HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) && previousState == 0x00)
    {
        previousState = 0xFF;
        key = 0;  // это то самое число "0", что мы указали в Interactions
        return true;
    }
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        previousState = 0x00;
    }
    return false;
}
