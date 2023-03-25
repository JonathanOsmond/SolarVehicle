#include "BCInputInterface.hpp"
#include "BCConfig.hpp"
#include "Debug.hpp"

BCInputInterface::BCInputInterface(Callback<void(current_t)> _handleCurrent,
        Callback<void(voltage_t)> _handleVoltage) :
    i2c(PinDefs::ADC_I2C_SDA, PinDefs::ADC_I2C_SCL),
    adc(&i2c), handleCurrent(_handleCurrent), handleVoltage(_handleVoltage) {
    }

void BCInputInterface::trigger() {
    adc.setGain(GAIN_EIGHT);
//    int16_t current = Config::PACK_CURRENT_OFFSET - adc.readADC_Differential_0_1() ;
    int16_t current = shuntoffset - adc.readADC_Differential_0_1() ;

//    DEBUG("ADC current: %d (%d mA) Off:%d", current, current * Config::PACK_CURRENT_SCALING,shuntoffset);

    handleCurrent(current * Config::PACK_CURRENT_SCALING);
    adc.setGain(GAIN_ONE);
    uint16_t voltage = adc.readADC_SingleEnded(3); // TODO: Check channel

    //DEBUG("ADC voltage: %hi (%i mV)", voltage, (voltage_t)(voltage * Config::PACK_VOLTAGE_SCALING));

    handleVoltage(voltage * Config::PACK_VOLTAGE_SCALING);
}

void BCInputInterface::calshunt() {
    adc.setGain(GAIN_EIGHT);
	shuntoffset = adc.readADC_Differential_0_1() ;
}