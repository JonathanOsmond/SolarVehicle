#include "BCOutputInterface.hpp"

constexpr uint16_t FAN_PWM_PERIOD_US = 1000;

BCOutputInterface::BCOutputInterface() :
#ifdef FAN1
    fan1(PinDefs::FAN1_DRIVE),
#endif
    fan2(PinDefs::FAN2_DRIVE) {
#ifdef FAN1
    fan1.period_us(FAN_PWM_PERIOD_US);
#endif
    fan2.period_us(FAN_PWM_PERIOD_US);
}

bool BCOutputInterface::setGndContactor(bool state) {
    return conGround.setState(state);
}

bool BCOutputInterface::setPrechargeContactor(bool state) {
    return conPrecharge.setState(state);
}

bool BCOutputInterface::setPositiveContactor(bool state) {
    return conPositive.setState(state);
}

bool BCOutputInterface::setChargeContactor(bool state) {
    return conCharge.setState(state);
}


bool BCOutputInterface::shutdown() {
    conCharge.setState(false);

    Thread::wait(Config::MPPT_CONTACTOR_DELAY);

    conPositive.fastSet(false);
    conPrecharge.fastSet(false);
    conGround.fastSet(false);

    return conPositive.waitSwitch(false)
        && conPrecharge.waitSwitch(false)
        && conGround.waitSwitch(false);
}

void BCOutputInterface::setFan1(uint8_t speed) {
#ifdef FAN1
    fan1.write(speed / 255.0);
#endif
}

void BCOutputInterface::setFan2(uint8_t speed) {
    fan2.write(speed / 255.0);
}
