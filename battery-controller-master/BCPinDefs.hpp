#ifndef BC_PIN_DEFS_HPP
#define BC_PIN_DEFS_HPP

#include <mbed.h>

namespace PinDefs {
    constexpr PinName CMU_SPI_MOSI = p5;
    constexpr PinName CMU_SPI_MISO = p6;
    constexpr PinName CMU_SPI_SCLK = p7;
    constexpr PinName CMU_SPI_CS = p8;

    constexpr PinName ADC_I2C_SDA = p9;
    constexpr PinName ADC_I2C_SCL = p10;

    constexpr PinName FAN1_SENSE = p24;
    constexpr PinName FAN2_SENSE = p23;
    constexpr PinName FAN1_DRIVE = p22;
    constexpr PinName FAN2_DRIVE = p21;

    constexpr PinName CON1_SENSE = p20;
    constexpr PinName CON2_SENSE = p19;
    constexpr PinName CON3_SENSE = p18;
    constexpr PinName CON1_DRIVE = p27;
    constexpr PinName CON2_DRIVE = p26;
    constexpr PinName CON3_DRIVE = p25;
    constexpr PinName CON4_DRIVE = FAN1_DRIVE;
    constexpr PinName CON_ENABLE = p11;

    constexpr PinName PAUSE_RESUME = p12;

    constexpr PinName CAN_RS = p28;
    constexpr PinName CAN_RX = p30;
    constexpr PinName CAN_TX = p29;
}
#endif
