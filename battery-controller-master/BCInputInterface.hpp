#ifndef BC_INPUT_INTERFACE_HPP
#define BC_INPUT_INTERFACE_HPP

#include "BCPinDefs.hpp"
#include "BCTypes.hpp"
#include <mbed.h>
#include <ADS1015/Adafruit_ADS1015.h>

class BCInputInterface {
    public:
		int shuntoffset;
        /** Interface to BC ADC for reading pack current and voltage
         * @param _handleCurrent Callback for new current reading
         * @param _handleVoltage Callback for new voltage reading
         */
        BCInputInterface(Callback<void(current_t)> _handleCurrent,
                Callback<void(voltage_t)> _handleVoltage);

        /** Initiate a voltage and current reading */
        void trigger();
		void calshunt();

    private:
        I2C i2c;
        Adafruit_ADS1115 adc;

        Callback<void(current_t)> handleCurrent;
        Callback<void(voltage_t)> handleVoltage;
};
#endif
