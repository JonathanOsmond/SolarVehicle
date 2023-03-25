#ifndef BC_STATE_MACHINE
#define BC_STATE_MACHINE

#include "BCTypes.hpp"
#include "BCConfig.hpp"
#include "BCOutputInterface.hpp"
#include "Debug.hpp"
#include "CANInterface.hpp"
#include "BCCANPackets.hpp"

#include <mbed.h>

class _AbstractState;

class BCStateMachine {
    public:
        enum State {
            BC_IDLE,
            BC_PRECHARGE,
            BC_RUN,
            BC_CHARGED,
            BC_BALANCE,
            BC_ERROR,
            // Not actually a state, but used to reset BC to idle
            BC_ERROR_UNLOCK = 255
        };

        /** Construct a new state machine.
         * @param cani CAN interface over which messages will be sent
         */
        BCStateMachine(CANInterface & cani);

        /** Update current pack charge/discharge current
         * @param current Current current
         */
        void setCurrent(current_t current);

        /** Update current pack voltage - only required during precharge, comes from CMUs
         * @param voltage Current voltage
         */
        void setPackVoltage(voltage_t voltage);

        /** Update current car voltage */
        void setCarVoltage(voltage_t voltage);

        /** Update function, must be called regularly.
         *
         * Polls for CAN, checks time-related stuff
         */
        void tick();

        /** Force a state transition.  Use with care! */
        void forceTransition(State state);

        State getState();

        BCOutputInterface output;

		void handleCAN_dispatch();

        /** Handle transitions that can be caused by CMU cell voltage */
		void handleCellVoltage(voltage_t voltage_min, voltage_t voltage_max);


		private:
        /** Handle incoming CAN message
         * @param msg Incoming CAN message
         */
        void handleCAN(const CANMessage & msg);

        /** Handle transitions that can be caused by CMU voltage or car voltage */
        void handleVoltage(voltage_t voltage, bool pack);


        /** Transition to a new state and apply some entry/exit conditions */
        void transition(State state);

        /** Current car state */
        State state;

        /** Convert state to a printable string constant. **/
        const char * stateName(State state);

        time_t last_ticker;
        time_t current_time; // ms
        time_t last_transition; // ms
        time_t last_heartbeat; // ms
        time_t last_group1; // ms - faster CAN messages
        time_t last_group2; // ms - slower CAN messages

        CANInterface & can;

        voltage_t lastCarVoltage;
        voltage_t lastPackVoltage;
        current_t lastCurrent;

        BCCANPackets::TX::Issue issue;
		
		char horn_flag;
};

#endif
