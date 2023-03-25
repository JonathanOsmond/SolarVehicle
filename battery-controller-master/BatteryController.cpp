#include "BatteryController.hpp"
#include "BCPinDefs.hpp"
#include "IOTemplates.hpp"
#include "Debug.hpp"
#include "BCConfig.hpp"
#include "BCCANPackets.hpp"

#include <limits.h>

BatteryController::BatteryController() :
    can(PinDefs::CAN_RX, PinDefs::CAN_TX, PinDefs::CAN_RS, Config::CAN_TX_BASE),
    stateMachine(can),
    input(Callback<void(current_t)>(&stateMachine, &BCStateMachine::setCurrent),
            Callback<void(voltage_t)>(&stateMachine, &BCStateMachine::setCarVoltage)),
    cmu_send_counter(0), averagedPackVoltage(-1)
	{
        can.frequency(500000);

	//************Attach CAN RX intterupt to handler.*************
	//	can.attach(Callback<void(void)>(&stateMachine, &BCStateMachine::handleCAN_dispatch), CAN::RxIrq);
	//************************************************************
	
	//CAN Acceptance Filter (other than following IDare filtered and ignored
		CAN2_wrFilter (Config::CAN_RX_BASE + BCCANPackets::RX::Heartbeat::ID);
		CAN2_wrFilter (Config::CAN_RX_BASE + BCCANPackets::RX::StateChange::ID);
		CAN2_wrFilter (Config::CAN_RX_BASE + BCCANPackets::RX::HMIStatus::ID);
	}

void BatteryController::run() {
    while(true) {
        stateMachine.tick();
        input.trigger();
        updatePackVoltage();
        Thread::wait(5);
    }
}

void BatteryController::sendCAN(const CANMessage & msg) {
    DEBUG_CAN("TX", msg);
    if(can.write(msg) == 0)
        ERROR("CAN write failed!");
}

void BatteryController::updatePackVoltage() {
    voltage_t packVoltage = 0;

    BCCANPackets::TX::CMUReading canmsg;

    voltage_t vmin = INT_MAX;
    int vmini = -1;
    voltage_t vmax = INT_MIN;
    int vmaxi = -1;

//Send all the cell voltage to CAN & detects minimum and maximum cell voltage


    if(cmu_send_counter >= 200) {
		cmu.doCellBalance();
        cmu.doCellConversion();
        cmu.doTempConversion();
        cmu_send_counter = 0;
        for(int cmuc=0; cmuc < Config::NUM_CMUs; ++cmuc) {
            DEBUG_ARRAY("CMU voltages", "%hu", cmu.cell_codes[cmuc], 12);
            for(int cell=0; cell < 12; cell += 2) { //*# Starts the count at 0 and goes 11. Hence 12 battery packs are being read. 
                for(int i=0; i<2; ++i) {
                    canmsg.cell_id[i] = cell + 12*cmuc + i;
                    voltage_t voltage = cmu.cell_codes[cmuc][cell + i]/10; // mV
                    canmsg.cell_voltage[i] = voltage;
                    packVoltage += voltage;
					canmsg.cell_temperature[i] = cmu.temp_scaled[cmuc][cell + i];
					
                    if(voltage < vmin) {
                        vmini = i + cell + cmuc * Config::NUM_CMUs;
                        vmin = voltage;
                    }
                    if(voltage > vmax) {
                        vmaxi = i + cell + cmuc * Config::NUM_CMUs;
                        vmax = voltage;
                    }
                }
                can.send(&canmsg);
            }
        }
        DEBUG("Min, max, average: %i, %i, %.0f", vmin, vmax, packVoltage / (12.0f * Config::NUM_CMUs));
    } else {
        cmu.doCellConversion();
		for(int cmuc=0; cmuc < Config::NUM_CMUs; ++cmuc) {
            for(int cell=0; cell < 12; cell++) {
                voltage_t voltage = cmu.cell_codes[cmuc][cell]/10;
                packVoltage += voltage;
                if(voltage < vmin) {
                    vmini = cell + cmuc * Config::NUM_CMUs;
                    vmin = voltage;
                }
                if(voltage > vmax) {
                    vmaxi = cell + cmuc * Config::NUM_CMUs;
                    vmax = voltage;
                }
            }
        }
    }

/*
    if(vmin < Config::MIN_CELL_VOLTAGE && stateMachine.getState() != BCStateMachine::BC_ERROR && stateMachine.getState() != BCStateMachine::BC_IDLE) {
        ERROR("Min cell (%i) is too low with voltage %i", vmini, vmin);
        stateMachine.forceTransition(BCStateMachine::BC_ERROR);
    }
    if(vmax > Config::MAX_CELL_VOLTAGE && stateMachine.getState() != BCStateMachine::BC_ERROR && stateMachine.getState() != BCStateMachine::BC_IDLE) {
        ERROR("Max cell (%i) is too high with voltage %i", vmaxi, vmax);
        //stateMachine.forceTransition(BCStateMachine::BC_ERROR);
    }
*/

	stateMachine.handleCellVoltage(vmin,vmax);

    ++cmu_send_counter;


// Loading initial pack voltage value for the first time.
	if(averagedPackVoltage < 0) {
        stateMachine.setPackVoltage(packVoltage);
        return;
    } else {
        averagedPackVoltage -= averagedPackVoltage / Config::PACK_VOLTAGE_AVERAGE_N;
        averagedPackVoltage += 1.0f * packVoltage / Config::PACK_VOLTAGE_AVERAGE_N;
    }

    stateMachine.setPackVoltage(averagedPackVoltage);
}
