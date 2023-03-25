#ifndef BC_CAN_PACKETS_HPP
#define BC_CAN_PACKETS_HPP

#include <mbed.h>

/** CAN Protocol definition for battery controller.
 *
 * Note that definitions of CAN IDs are relative to the base addresses specified in BCConfig
 */

#define _CANID(id) enum { ID = id }
namespace BCCANPackets {
    namespace TX {
        /** Heartbeat demonstrating battery controller is still alive. */
        struct Heartbeat {
            _CANID(0x0);
            enum { MAGIC = 0x43617473 };
            uint32_t magic_number;
            uint8_t state; // Current car state - see BCStateMachine::State
        };

        /** Pack and car voltages. */
        struct PackVoltage {
            _CANID(0x1);
            int32_t packVoltage; // mV
            int32_t carVoltage; // mV
        };

        /** Pack current as measured by the shunt. */
        struct PackCurrent {
            _CANID(0x2);
            int32_t packCurrent; // mA
        };

        /** Something went wrong :( */
        struct Issue {
            _CANID(0x3);
            uint32_t whatWentWrong;
            enum {
                OK = 0,
                UNDERVOLTAGE = 1 << 0, // Pack under preferred minimum voltage
                OVERVOLTAGE = 1 << 1, // Pack over preferred maximum voltage
                UNDER_VOLTAGE_LOCKOUT = 1 << 2, // Pack under absolute minimum voltage
                OVER_VOLTAGE_LOCKOUT = 1 << 3, // Pack over absolute maximum voltage
                OVER_CHARGE_CURRENT = 1 << 4, // Pack over charging limit
                OVER_DISCHARGE_CURRENT = 1 << 5, // Pack over discharging limit
                CONTACTOR = 1 << 6, // Contactor failed to switch
                OVER_TEMPERATURE = 1 << 7,
                UNDER_TEMPERATURE = 1 << 8,
                PRECHARGE_FAIL = 1 << 9,
                HEARTBEAT_TIMEOUT = 1 << 10,
                UNKNOWN = 1 << 31
            };
        };

        struct CMUReading {
            _CANID(0x4);
            uint8_t cell_id[2];
            uint16_t cell_voltage[2]; // 1/10 mV
            uint8_t cell_temperature[2]; // C
        };

        struct ChargeState {
            _CANID(0x5);
            float percentage;
            float amp_hours;
        };
    }

    namespace RX {
        struct Heartbeat {
            _CANID(0x0);
            enum { MAGIC = 0x536F6C72 };
            uint32_t magic_number;
        };

        struct StateChange {
            _CANID(0x21);
            uint8_t newstate;
        };
 
		struct HMIStatus {
            _CANID(0x01);
            uint8_t buttons;
        };
		
		
    }

}

#endif
