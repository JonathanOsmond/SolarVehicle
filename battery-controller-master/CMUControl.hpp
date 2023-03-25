#ifndef CMU_CONTROL_HPP
#define CMU_CONTROL_HPP

#include <mbed.h>
#include "BCConfig.hpp"
#include "IOTemplates.hpp"
#include "BCPinDefs.hpp"

namespace CMUConstants {
    /**
      |MD| Dec  | ADC Conversion Model|
      |--|------|---------------------|
      |01| 1    | Fast                |
      |10| 2    | Normal              |
      |11| 3    | Filtered            |
      */
    constexpr uint8_t MD_FAST = 1;
    constexpr uint8_t MD_NORMAL = 2;
    constexpr uint8_t MD_FILTERED = 3;


    /**
      |CH | Dec  | Channels to convert |
      |---|------|---------------------|
      |000| 0    | All Cells           |
      |001| 1    | Cell 1 and Cell 7   |
      |010| 2    | Cell 2 and Cell 8   |
      |011| 3    | Cell 3 and Cell 9   |
      |100| 4    | Cell 4 and Cell 10  |
      |101| 5    | Cell 5 and Cell 11  |
      |110| 6    | Cell 6 and Cell 12  |
      */

    constexpr uint8_t CELL_CH_ALL = 0;
    constexpr uint8_t CELL_CH_1and7 = 1;
    constexpr uint8_t CELL_CH_2and8 = 2;
    constexpr uint8_t CELL_CH_3and9 = 3;
    constexpr uint8_t CELL_CH_4and10 = 4;
    constexpr uint8_t CELL_CH_5and11 = 5;
    constexpr uint8_t CELL_CH_6and12 = 6;


    /**
      |CHG | Dec  |Channels to convert   |
      |----|------|----------------------|
      |000 | 0    | All GPIOS and 2nd Ref|
      |001 | 1    | GPIO 1               |
      |010 | 2    | GPIO 2               |
      |011 | 3    | GPIO 3               |
      |100 | 4    | GPIO 4               |
      |101 | 5    | GPIO 5               |
      |110 | 6    | Vref2                |
      */

    constexpr uint8_t AUX_CH_ALL = 0;
    constexpr uint8_t AUX_CH_GPIO1 = 1;
    constexpr uint8_t AUX_CH_GPIO2 = 2;
    constexpr uint8_t AUX_CH_GPIO3 = 3;
    constexpr uint8_t AUX_CH_GPIO4 = 4;
    constexpr uint8_t AUX_CH_GPIO5 = 5;
    constexpr uint8_t AUX_CH_VREF2 = 6;

    /** Controls if discharging transistors are enabled
      or disabled during adc conversions.

      |DCP | Discharge Permitted During conversion |
      |----|---------------------------------------|
      |0   | No - discharge is not permitted       |
      |1   | Yes - discharge is permitted          |
      */
    constexpr uint8_t DCP_DISABLED = 0;
    constexpr uint8_t DCP_ENABLED = 1;

    constexpr uint8_t I2CSTART = 0x06; //0b0110
    constexpr uint8_t I2CSTOP = 0x01; //0b0001
    constexpr uint8_t I2CBLANK = 0x00; //0b0000
    constexpr uint8_t I2CNOTRANSMIT = 0x07; //b0111
    constexpr uint8_t I2CACK = 0x00; //0b0000
    constexpr uint8_t I2CNACK = 0x08; //0b1000
    constexpr uint8_t I2CNACKSTOP = 0x09; //0b1001

    constexpr uint8_t SPICSLOW = 0x08;
    constexpr uint8_t SPICSHIGH = 0x09;
    constexpr uint8_t SPINOTRANSMIT = 0x0f;
    constexpr uint8_t BALANCE_SET = 0xa9;
	
    constexpr uint16_t THERMISTOR_CONST[10] = {22951,19956,16660,13387,10431,7951,5981,4475,3348,2515}; 
	                                            //0   10   20   30   40  50  60  70  80  90
}

class CMUControl {
    public:
        CMUControl();

        void doCellConversion();
        void doTempConversion();
        void doCellBalance();
		uint8_t TempScaling(uint16_t v_reading);
		
        /** Cell voltages in 1/10 mV **/
        uint16_t cell_codes[Config::NUM_CMUs][12];
        uint16_t temp_codes[Config::NUM_CMUs][16];
        uint8_t temp_scaled[Config::NUM_CMUs][12];
//       uint8_t temp_scaled[12 * Config::NUM_CMUs];
		
    private:
        void wrcfg(uint8_t config[][6]);
        
        /** Maps global ADC control variables to the appropriate control
         * bytes for each of the different ADC commands
         *
         * @param[in] uint8_t MD The adc conversion mode
         * @param[in] uint8_t DCP Controls if Discharge is permitted during cell conversions
         * @param[in] uint8_t CH Determines which cells are measured during an ADC conversion command
         * @param[in] uint8_t CHG Determines which GPIO channels are measured during Auxiliary conversion command
         *
         * Command Code:
         * -------------
         *  |CMD[0:1]| 15 | 14 | 13 | 12 | 11 | 10 | 9 |   8   |   7   | 6 | 5 |  4  | 3 |   2   |   1   |   0   |
         *  |--------|----|----|----|----|----|----|---|-------|-------|---|---|-----|---|-------|-------|-------|
         *  |ADCV:   |  0 |  0 |  0 |  0 |  0 |  0 | 1 | MD[1] | MD[2] | 1 | 1 | DCP | 0 | CH[2] | CH[1] | CH[0] |
         *  |ADAX:   |  0 |  0 |  0 |  0 |  0 |  1 | 0 | MD[1] | MD[2] | 1 | 1 | DCP | 0 | CHG[2]| CHG[1]| CHG[0]|
         */
        void set_adc(uint8_t MD, uint8_t DCP, uint8_t CH, uint8_t CHG);

        /** Starts cell voltage conversion
         * Starts ADC conversions of the LTC6804 Cpin inputs.
         * The type of ADC conversion executed can be changed by setting the associated global variables
         * |Variable|Function                                      |
         * |--------|----------------------------------------------|
         * | MD     | Determines the filter corner of the ADC      |
         * | CH     | Determines which cell channels are converted |
         * | DCP    | Determines if Discharge is Permitted         |
         *
         * Command Code:
         * -------------
         *
         * |CMD[0:1]| 15 | 14 | 13 | 12 | 11 | 10 | 9 |   8   |   7   | 6 | 5 |  4  | 3 |   2   |   1   |   0   |
         * |--------|----|----|----|----|----|----|---|-------|-------|---|---|-----|---|-------|-------|-------|
         * |ADCV:   |  0 |  0 |  0 |  0 |  0 |  0 | 1 | MD[1] | MD[2] | 1 | 1 | DCP | 0 | CH[2] | CH[1] | CH[0] |
         */
        void adcv();

        /** Start an GPIO Conversion
         * Starts an ADC conversions of the LTC6804 GPIO inputs.
         * The type of ADC conversion executed can be changed by setting the associated global variables
         * |Variable|Function                                      |
         * |--------|----------------------------------------------|
         * | MD     | Determines the filter corner of the ADC      |
         * | CHG    | Determines which GPIO channels are converted |
         * Command Code:
         * -------------
         *
         *  |CMD[0:1] |  15   |  14   |  13   |  12   |  11   |  10   |   9   |   8   |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
         *  |---------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
         *  |ADAX:    |   0   |   0   |   0   |   0   |   0   |   1   |   0   | MD[1] | MD[2] |   1   |   1   |  DCP  |   0   | CHG[2]| CHG[1]| CHG[0]|
         */
        void adax();

        /** Reads and parses the LTC6804 cell voltage registers.
         * The function is used to read the cell codes of the LTC6804.
         * This function will send the requested read commands parse the data
         * and store the cell voltages in cell_codes variable.
         *
         * @param[in] reg This controls which cell voltage register is read back.
         *      0: Read back all Cell registers
         *      1: Read back cell group A
         *      2: Read back cell group B
         *      3: Read back cell group C
         *      4: Read back cell group D
         * @param[in] total_ic This is the number of ICs in the daisy chain(-1 only)
         * @param[out] cell_codes An array of the parsed cell codes from lowest to highest. The cell codes will
         * be stored in the cell_codes[] array in the following format:
         * |  cell_codes[0][0]| cell_codes[0][1] |  cell_codes[0][2]|    .....     |  cell_codes[0][11]|  cell_codes[1][0] | cell_codes[1][1]|  .....   |
         * |------------------|------------------|------------------|--------------|-------------------|-------------------|-----------------|----------|
         * |IC1 Cell 1        |IC1 Cell 2        |IC1 Cell 3        |    .....     |  IC1 Cell 12      |IC2 Cell 1         |IC2 Cell 2       | .....    |
         *
         * @return int8_t, PEC Status.
         *      0: No PEC error detected
         *      -1: PEC error detected, retry read
         */
        int8_t rdcv(uint8_t reg);

        /** Read the raw data from the LTC6804 cell voltage register
         *
         * The function reads a single cell voltage register and stores the read data
         * in the *data point as a byte array. This function is rarely used outside of
         * the LTC6804_rdcv() command.
         *
         * @param[in] uint8_t reg; This controls which cell voltage register is read back.
         *  1: Read back cell group A
         *  2: Read back cell group B
         *  3: Read back cell group C
         *  4: Read back cell group D
         *
         * @param[in] uint8_t NUM_CMUs; This is the number of ICs in the daisy chain(-1 only)
         * @param[out] uint8_t *data; An array of the unparsed cell codes
         *
         * Command Code:
         * -------------
         *
         * |CMD[0:1] |  15   |  14   |  13   |  12   |  11   |  10   |   9   |   8   |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
         * |---------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
         * |RDCVA:   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   1   |   0   |   0   |
         * |RDCVB:   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   1   |   1   |   0   |
         * |RDCVC:   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   1   |   0   |   0   |   0   |
         * |RDCVD:   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   1   |   0   |   1   |   0   |
         */
        void rdcv_reg(uint8_t reg, uint8_t *data);

        /** Reads and parses the LTC6804 auxiliary registers.
         *
         * The function is used to read the  parsed GPIO codes of the LTC6804.
         * This function will send the requested read commands parse the data
         * and store the gpio voltages in aux_codes variable
         *
         * @param[in] uint8_t reg; This controls which GPIO voltage register is read back.
         *      0: Read back all auxiliary registers
         *      1: Read back auxiliary group A
         *      2: Read back auxiliary group B
         *
         * @param[out] uint16_t aux_codes[][6]; A two dimensional array of the gpio voltage codes.
         * The GPIO codes will be stored in the aux_codes[][6] array in the following format:
         *
         * |  aux_codes[0][0]| aux_codes[0][1] |  aux_codes[0][2]|  aux_codes[0][3]|  aux_codes[0][4]|  aux_codes[0][5]| aux_codes[1][0] |aux_codes[1][1]|  .....    |
         * |-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|---------------|-----------|
         * |IC1 GPIO1        |IC1 GPIO2        |IC1 GPIO3        |IC1 GPIO4        |IC1 GPIO5        |IC1 Vref2        |IC2 GPIO1        |IC2 GPIO2      |  .....    |
         *
         * @return  int8_t, PEC Status
         *      0: No PEC error detected
         *      -1: PEC error detected, retry read
         */
        int8_t rdaux(uint8_t reg, uint16_t aux_codes[][6]);

        /** Read the raw data from the LTC6804 auxiliary register
         * The function reads a single GPIO voltage register and stores thre read data
         * in the *data point as a byte array. This function is rarely used outside of
         * the LTC6804_rdaux() command.
         * @param[in] uint8_t reg; This controls which GPIO voltage register is read back.
         *      1: Read back auxiliary group A
         *      2: Read back auxiliary group B
         *
         * @param[out] uint8_t *data; An array of the unparsed aux codes
         *
         * Command Code:
         * -------------
         *
         *  |CMD[0:1]     |  15   |  14   |  13   |  12   |  11   |  10   |   9   |   8   |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
         *  |---------------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
         *  |RDAUXA:      |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   1   |   1   |   0   |   0   |
         *  |RDAUXB:      |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   0   |   1   |   1   |   1   |   0   |
         */
        void rdaux_reg(uint8_t reg, uint8_t *data);

        /** Set analog muxes to correct channel - sets all muxes to the same channel.
         *
         * @param channel Channel to set mux to - 0-7.
         */
        void adc_mux(uint8_t channel);

        /** Set I2C or SPI bus command.
         *
         * @author Norio Itsumi
         *
         * @param config Array of data to be written
         */
        void wrcom(uint8_t config[][6]);

        /** Execute I2C or SPI bus command.
         *
         * @author Norio Itsumi
         */
        void excom();

        /** Ensure LTC6804 is awake from sleep mode */
        void wakeup_sleep();

        /** Ensure LTC6804 is awake from idle mode */
        void wakeup_idle();

        SPI spi;

        /** Cell Voltage conversion command. */
        uint8_t ADCV[2];
        /** GPIO conversion command. */
        uint8_t ADAX[2];

        /** GPIOs 1-5 and Vref2 voltages in 1/10 mV **/
        uint16_t aux_codes[Config::NUM_CMUs][6];

        uint8_t tx_cfg[Config::NUM_CMUs][6];
        uint8_t rx_cfg[Config::NUM_CMUs][8];
        /** Communication control command */
        uint8_t com_codes[Config::NUM_CMUs][6];
        uint8_t config_codes[Config::NUM_CMUs][6];

        void setCS() {
            IOTemplates::set<PinDefs::CMU_SPI_CS>();
        }

        void clearCS() {
            IOTemplates::clear<PinDefs::CMU_SPI_CS>();
        }

        uint16_t pec15_calc(uint8_t len, const uint8_t *data);

        void spi_write_array(uint8_t len, const uint8_t *data);
        void spi_write_read(const uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t rx_len);
};
#endif
