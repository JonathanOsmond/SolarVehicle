#include "BCStateMachine.hpp"
#include "hal/us_ticker_api.h"

#define TRANSITION(state) do { DEBUG("State change to: " #state); transition(state); } while(false)

using namespace BCCANPackets;

BCStateMachine::BCStateMachine(CANInterface & cani) :
    state(BC_IDLE),
    last_ticker(0),
    current_time(0),
    last_transition(0),
    last_heartbeat(0),
    last_group1(0),
    last_group2(0),
    can(cani),
    lastCarVoltage(0),
    lastPackVoltage(0),
    lastCurrent(0) {
        last_ticker = us_ticker_read();
        TRANSITION(BC_IDLE);
    }

void BCStateMachine::setCurrent(current_t current) {
    lastCurrent = current;

    if(state == BC_ERROR)
        return;

    if(current < Config::MAX_DISCHARGE_CURRENT) {
        TRANSITION(BC_ERROR);
        ERROR("Discharge current limit exceeded: %i mA", current);
        issue.whatWentWrong |= TX::Issue::OVER_DISCHARGE_CURRENT;
        return;
    }

    if(current > Config::MAX_CHARGE_CURRENT) {
        TRANSITION(BC_ERROR);
        ERROR("Charge current limit exceeded: %i mA", current);
        issue.whatWentWrong |= TX::Issue::OVER_CHARGE_CURRENT;
        return;
    }
}

void BCStateMachine::setPackVoltage(voltage_t voltage) {
    lastPackVoltage = voltage;

    if(state == BC_ERROR)
        return;

//    handleVoltage(voltage, true);
}

void BCStateMachine::setCarVoltage(voltage_t voltage) {
    lastCarVoltage = voltage;
    
    if(state == BC_ERROR)
        return;

//    if(state == BC_RUN || state == BC_CHARGED || state == BC_BALANCE)
//       handleVoltage(voltage, false);

    if(voltage > Config::PRECHARGE_COMPLETE_PERCENTAGE * lastPackVoltage && state == BC_PRECHARGE) {
         if(current_time - last_transition > 500) {
            wait_ms(200);
            output.setPositiveContactor(true);
            wait_ms(50);
            output.setPrechargeContactor(false);
            wait_ms(500);
            output.setChargeContactor(true);
            TRANSITION(BC_RUN);
        }
    }
}

//****************CAN RX Interrupt handler******************
void BCStateMachine::handleCAN_dispatch(void) {
    CANMessage msg;
	can.read(msg);		//Read can message from CANRX registers
    handleCAN(msg);		//hand it to CAN message decoder.
}


void BCStateMachine::handleCAN(const CANMessage & msg) {
    // Ignore all IDs smaller than the base address
    if(msg.id < Config::CAN_RX_BASE)
        return;

    switch(msg.id - Config::CAN_RX_BASE) {
        case RX::StateChange::ID:
            {
                if(msg.len != sizeof(RX::StateChange)) {
                    WARN("Malformed CAN: StateChange with ID %x must have length %lu",
                            msg.id, sizeof(RX::StateChange));
                    break;
                }
                RX::StateChange * s = (RX::StateChange*)msg.data;

                switch(s->newstate) {
                    case BC_IDLE:
                        if(state != BC_ERROR)
                            TRANSITION(BC_IDLE);
                        break;
                    case BC_RUN:
                        if(state == BC_IDLE)
                            TRANSITION(BC_PRECHARGE);
                        break;
                    case BC_ERROR:
                        TRANSITION(BC_ERROR);
                        ERROR("CAN-initiated error state!");
                        issue.whatWentWrong |= TX::Issue::UNKNOWN;
                        break;
                    case BC_ERROR_UNLOCK:
                        if(state == BC_ERROR) {
                            WARN("CAN forced state from error to idle!");
                            IOTemplates::clear<LED1>();
                            IOTemplates::clear<LED2>();
                            IOTemplates::clear<LED3>();
                            IOTemplates::clear<LED4>();

                            INFO("Clearing error flags");
                            issue.whatWentWrong = TX::Issue::OK;
                            TRANSITION(BC_IDLE);
                        }
                    default:
                        WARN("Malformed CAN: got state change request to %hhu", s->newstate);
                        break;
                }
            }
            break;
        case RX::Heartbeat::ID: // Heartbeat
            {
                if(msg.len != sizeof(RX::Heartbeat)) {
                    WARN("Malformed CAN: Heartbeat with ID %x must have length %lu",
                            msg.id, sizeof(RX::Heartbeat));
                    break;
                }
                RX::Heartbeat * hb = (RX::Heartbeat*)msg.data;

                if(hb->magic_number != RX::Heartbeat::MAGIC) {
                    WARN("Malformed CAN: Incoming heartbeat has incorrect magic number of %X",
                            hb->magic_number);
                    break;
                }

                //DEBUG("Got Heartbeat!");

                last_heartbeat = current_time;

                issue.whatWentWrong &= ~TX::Issue::HEARTBEAT_TIMEOUT;
            }
            break;
		case RX::HMIStatus::ID: // HMI (Stop voltage check during using Horn due to noise issue)
            {
                if(msg.len != sizeof(RX::HMIStatus)) {
                    WARN("Malformed CAN: HMIStatus with ID %x must have length %lu",
                            msg.id, sizeof(RX::HMIStatus));
                    break;
                }
                RX::HMIStatus * btn = (RX::HMIStatus*)msg.data;

                if(btn->buttons & 0x04) {  ///Horn status
					 horn_flag = 1;
				}else {
					horn_flag = 0;
				} 
             }
            break;
        default: // If the ID isn't handled above, ignore it.
            break;
    }
}

BCStateMachine::State BCStateMachine::getState() {
    return state;
}

void BCStateMachine::tick() {
    time_t new_ticker = us_ticker_read();
    current_time += (new_ticker - last_ticker)/1000;
    last_ticker = new_ticker;

    if(state != BC_IDLE && state != BC_ERROR && current_time - last_heartbeat > Config::HEARTBEAT_PERIOD) {
        TRANSITION(BC_IDLE);
        INFO("Heartbeat timeout! Diff: %li", (current_time - last_heartbeat));
        issue.whatWentWrong |= TX::Issue::HEARTBEAT_TIMEOUT;
    }

    if(state == BC_PRECHARGE) {
        if(current_time - last_transition > Config::PRECHARGE_ERROR_PERIOD) {
            TRANSITION(BC_ERROR);
            ERROR("Precharge failed!");
            issue.whatWentWrong |= TX::Issue::PRECHARGE_FAIL;
        }
    }

    if(current_time - last_group1 > Config::CAN_GROUP1_PERIOD) {
        last_group1 = current_time;

        TX::PackVoltage pv;
        pv.packVoltage = lastPackVoltage;
        pv.carVoltage = lastCarVoltage;
        can.send(&pv);

        TX::PackCurrent pc;
        pc.packCurrent = lastCurrent;
        can.send(&pc);

        if(state == BC_ERROR) {
            IOTemplates::toggle<LED1>();
            IOTemplates::toggle<LED2>();
            IOTemplates::toggle<LED3>();
            IOTemplates::toggle<LED4>();
        }
		TX::ChargeState cs;
		float comp_voltage;
		comp_voltage = ((float)pv.packVoltage/36000) - ((float)pc.packCurrent / 11000 * 0.06);
		if(comp_voltage >= 4.2){
			cs.amp_hours = 11 * 3.2;
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 4.17){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 4.2303) / 0.437);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 4.14){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 4.197) / 0.206);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 4.12){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 4.183) / 0.149);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 4.09){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 4.216) / 0.229);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 3.5){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 4.215) / 0.262);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 3.485){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 4.021) / 0.195);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 3.32){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 5.28) / 0.653);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else if (comp_voltage > 3.0){
			cs.amp_hours = 11 * (3.2 + (comp_voltage - 6.516) / 1.065);
			cs.percentage = 100 * cs.amp_hours / 35.2;
		}else {
			cs.amp_hours = 0;
			cs.percentage = 0;
		}
        can.send(&cs);

/*
		Cell voltage = (-0.437 * battery_AH) +4.2303
		(Cell voltage - 4.2303)/-0.437 = battery used AH
*/		
		
		
	}
    if(current_time - last_group2 > Config::CAN_GROUP2_PERIOD) {
        last_group2 = current_time;

        // Send heartbeat
        TX::Heartbeat hb = { TX::Heartbeat::MAGIC, (uint8_t) state };
        can.send(&hb);
        
        if(issue.whatWentWrong != TX::Issue::OK) {
            can.send(&issue);
        }

        DEBUG("Pack voltage: %u mV, car voltage: %u mV, current: %u mA",
                lastPackVoltage, lastCarVoltage, lastCurrent);
	}
	//*********** Polling for CAN RX *******************
    CANMessage msg;
    while(can.read(msg)) {
        //DEBUG_CAN("Got message!", msg);
        handleCAN(msg);
	}
	//**************************************************
	
}

void BCStateMachine::forceTransition(State state) {
    transition(state);
    WARN("Transition forced to state %s!", stateName(state));
}


////--------------Need work to check individual cell under/over voltage. (compare with min and max voltage)


void BCStateMachine::handleVoltage(voltage_t voltage, bool pack) {
    // NB: This function is only called when not in the BC_ERROR state.
    // Also this function is not called on voltages that should be low,
    // for example the car voltage values before or during precharging.
    
    if(voltage > Config::OVER_PACK_VOLTAGE) {
        TRANSITION(BC_ERROR);
        ERROR("Over pack voltage!");
        DEBUG("%s voltage of %i mV is too high!", pack ? "Pack" : "Car", voltage);
        issue.whatWentWrong |= TX::Issue::OVER_VOLTAGE_LOCKOUT;
        return;
    }

    // XXX: Does not account for unbalanced cells
    if(voltage > Config::MAX_PACK_VOLTAGE && (state == BC_RUN || state == BC_BALANCE)) {
        output.setChargeContactor(false); // Stop charging
        TRANSITION(BC_CHARGED);
    }

    if(voltage < Config::CHARGE_CUTIN_PACK_VOLTAGE && state == BC_CHARGED) {
        DEBUG("Resume charging!");
        output.setChargeContactor(true); // Resume charging
        TRANSITION(BC_RUN);
    }

    // XXX: Does not account for unbalanced cells
    if(voltage < Config::UNDER_PACK_VOLTAGE) {
        TRANSITION(BC_ERROR);
        ERROR("Under pack voltage!");
        DEBUG("Under voltage: %i mV!", voltage);
        issue.whatWentWrong |= TX::Issue::UNDER_VOLTAGE_LOCKOUT;
        return;
    }

    // XXX: Does not account for unbalanced cells
    if(voltage < Config::MIN_PACK_VOLTAGE && (state != BC_IDLE && state != BC_RUN)) {
        issue.whatWentWrong |= TX::Issue::UNDERVOLTAGE;
    } else {
        issue.whatWentWrong &= ~TX::Issue::UNDERVOLTAGE;
    }
}


////--------------Need work to check individual cell under/over voltage. (compare with min and max voltage)
void BCStateMachine::handleCellVoltage(voltage_t voltage_min, voltage_t voltage_max) {
//         DEBUG("voltage Max %i mV, Min %i mV", voltage_max, voltage_min);

	if(horn_flag == 1) return; //When horn is turned on, skip cell voltage check due to noise isspace

    if(voltage_max > Config::OVER_CELL_VOLTAGE) {
//TRANSITION(BC_ERROR);
        ERROR("Over Cell voltage!");
        DEBUG("voltage of %i mV is too high!", voltage_max);
        issue.whatWentWrong |= TX::Issue::OVER_VOLTAGE_LOCKOUT;
        return;
    }

    if(voltage_max > Config::MAX_CELL_VOLTAGE && (state == BC_RUN || state == BC_BALANCE)) {
        DEBUG("Stop charging!");
        output.setChargeContactor(false); // Stop charging
        TRANSITION(BC_CHARGED);
    }

    if(voltage_max < Config::CHARGE_CUTIN_CELL_VOLTAGE && state == BC_CHARGED) {
        DEBUG("Resume charging!");
        output.setChargeContactor(true); // Resume charging
        TRANSITION(BC_RUN);
    }

    if(voltage_min < Config::UNDER_CELL_VOLTAGE) {
//       TRANSITION(BC_ERROR);
        ERROR("Under pack voltage!");
       DEBUG("Under voltage: %i mV!", voltage_min);
        issue.whatWentWrong |= TX::Issue::UNDER_VOLTAGE_LOCKOUT;
        return;
    }

    if(voltage_min < Config::MIN_CELL_VOLTAGE && (state != BC_IDLE && state != BC_RUN)) {
        issue.whatWentWrong |= TX::Issue::UNDERVOLTAGE;
    } else {
        issue.whatWentWrong &= ~TX::Issue::UNDERVOLTAGE;
    }
}


void BCStateMachine::transition(State state) {
    switch(state) {
        case BC_ERROR:
        case BC_IDLE:
            output.shutdown();
            break;
        case BC_PRECHARGE:
            output.setGndContactor(true);
            wait_ms(500);
            output.setPrechargeContactor(true);
        default:
            break;
    }
    this->state = state;
    last_transition = current_time;
}

const char * BCStateMachine::stateName(State state) {
#define SWITCHCASE(x) case x: return #x
    switch(state) {
        SWITCHCASE(BC_IDLE);
        SWITCHCASE(BC_PRECHARGE);
        SWITCHCASE(BC_RUN);
        SWITCHCASE(BC_CHARGED);
        SWITCHCASE(BC_BALANCE);
        SWITCHCASE(BC_ERROR);
        SWITCHCASE(BC_ERROR_UNLOCK);
    }
    return "Unknown state!";
}
