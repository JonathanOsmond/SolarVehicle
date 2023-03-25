#ifndef BATTERY_CONTROLLER_HPP
#define BATTERY_CONTROLLER_HPP

#include "BCInputInterface.hpp"
#include "BCStateMachine.hpp"
#include "CMUControl.hpp"
#include "CANInterface.hpp"
#include "canfilter.h"

class BatteryController {
    public:
        BatteryController();

        void run();

        CANInterface can;
        BCStateMachine stateMachine; //#*Object of BCStateMachine and is called stateMachine. 
        BCInputInterface input; //#*Object of BCInputInterface and is named input.
        CMUControl cmu; //#* Object of CMU Control and is named cmu. 
    private:
        void sendCAN(const CANMessage & msg);
        void updatePackVoltage();
        uint8_t cmu_send_counter;

        float averagedPackVoltage;
};

#endif
