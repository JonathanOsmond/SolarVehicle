#ifndef BC_CONFIG
#define BC_CONFIG
#include "BCTypes.hpp"
#include <mbed.h>

namespace Config {
    constexpr uint16_t NUM_CELLS_SERIES = 36;
    constexpr uint16_t NUM_CMUs = 3;

    constexpr current_t MAX_CHARGE_CURRENT = 50000; // mA
    constexpr current_t MAX_DISCHARGE_CURRENT = -50000; // mA

    constexpr voltage_t OVER_CELL_VOLTAGE = 4400; // mV: Voltage at wihch OPEN contactors
    constexpr voltage_t MAX_CELL_VOLTAGE = 4250; // mV : Voltage at which charging stops
    constexpr voltage_t CELL_BALANCE_VOLTAGE = 4230 ; // mV Balance discharge threshold
    constexpr voltage_t CHARGE_CUTIN_CELL_VOLTAGE = 4200; // mV Re-connect solar array
    constexpr voltage_t MIN_CELL_VOLTAGE = 3000; // mV : Voltage at which asserts warning
    constexpr voltage_t UNDER_CELL_VOLTAGE = 2700; // mV: Voltage at wihch OPEN contactors
 
    constexpr voltage_t MAX_PACK_VOLTAGE = MAX_CELL_VOLTAGE * NUM_CELLS_SERIES; // Voltage at which charging stops
    constexpr voltage_t OVER_PACK_VOLTAGE = OVER_CELL_VOLTAGE * NUM_CELLS_SERIES; // Voltage at which cutouts are triggered
    constexpr voltage_t MIN_PACK_VOLTAGE = MIN_CELL_VOLTAGE * NUM_CELLS_SERIES; // Voltage at which discharge flag is set
    constexpr voltage_t UNDER_PACK_VOLTAGE = UNDER_CELL_VOLTAGE * NUM_CELLS_SERIES; // Voltage at which cutouts are triggered
    constexpr voltage_t CHARGE_CUTIN_PACK_VOLTAGE = CHARGE_CUTIN_CELL_VOLTAGE * NUM_CELLS_SERIES; // Voltage at which charging restarts

    constexpr float PRECHARGE_COMPLETE_PERCENTAGE = 0.95; // Percentage of pack voltage that car voltage must reach before precharge is complete 

    // Used for both charging and discharging
    constexpr temperature_t MAX_CELL_TEMPERATURE = 650; // 1/10 C
    constexpr temperature_t MIN_CELL_TEMPERATURE = 100; // 1/10 C

    constexpr time_t MPPT_CONTACTOR_DELAY = 50; // ms

    constexpr unsigned int CAN_TX_BASE = 0x600;
    constexpr unsigned int CAN_RX_BASE = 0x200;
    constexpr unsigned int CAN_VCM_BASE = 0x200;
	
    constexpr time_t HEARTBEAT_PERIOD = 10000000; // ms
    constexpr time_t PRECHARGE_ERROR_PERIOD = 1000; // ms
    constexpr time_t CAN_GROUP1_PERIOD = 200; // ms
    constexpr time_t CAN_GROUP2_PERIOD = 1000; // ms

    constexpr uint16_t PACK_VOLTAGE_AVERAGE_N = 10; // Number of cells to average

	constexpr uint16_t CELL_CAPACITY = 3200; // Each Battery cell capacity

    // XXX: Requires calibration (Current and Voltage scale factor for ADC inputs)
    constexpr float PACK_CURRENT_SCALING = 3.263; // mA per ADC division
//    int PACK_CURRENT_OFFSET = 3; // ADC raw count offset at 0.0A
    constexpr int PACK_CURRENT_OFFSET = 0; // ADC raw count offset at 0.0A
    constexpr float PACK_VOLTAGE_SCALING = 2 * 6.28; // mV per ADC division


}

#endif
