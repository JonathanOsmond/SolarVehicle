#include "CMUControl.hpp"
#include "CRC15.hpp"
#include "Debug.hpp"

using namespace IOTemplates;
using namespace CMUConstants;
using Config::NUM_CMUs;
using Config::NUM_CELLS_SERIES;

constexpr uint16_t SPI_CS_DELAY = 35; // us

CMUControl::CMUControl() : spi(PinDefs::CMU_SPI_MOSI, PinDefs::CMU_SPI_MISO, PinDefs::CMU_SPI_SCLK) {
    makeOutput<PinDefs::CMU_SPI_CS>();

    // Setup the spi for 8 bit data, high steady state clock,
    // second edge capture, with a 1MHz clock rate
    spi.format(8,0);
    spi.frequency(1000000);

    memset(cell_codes, 255, NUM_CMUs * 12 * sizeof(uint16_t));

    // Set up ADC read commands
	set_adc(MD_FILTERED,DCP_DISABLED,CELL_CH_ALL,AUX_CH_ALL);
 //   set_adc(MD_NORMAL,DCP_DISABLED,CELL_CH_ALL,AUX_CH_ALL);  //for 7KHz

	
    // Initialise transmit config
    for(int i = 0; i<NUM_CMUs; ++i){
        tx_cfg[i][0] = 0xFD; // GPIO[1:5], REFON, SWTEN, ADCOPT.  Was 0xFE (0xFC for Norio)*************************************
//        tx_cfg[i][0] = 0xFC; // GPIO[1:5], REFON, SWTEN, ADCOPT=0.  Was 0xFE (0xFC for Norio)
        tx_cfg[i][1] = 0x00; // Under voltage threshold
        tx_cfg[i][2] = 0x00; // Under/over voltage threshold
        tx_cfg[i][3] = 0x00; // Over voltage threshold
        tx_cfg[i][4] = 0x00; // Cell discharge
        tx_cfg[i][5] = 0x00; // Cell discharge
    }
    wakeup_sleep();
    wrcfg(tx_cfg);
}

void CMUControl::doCellConversion() {
    wakeup_sleep();
    adcv();
    wait_ms(7);
    if(rdcv(CELL_CH_ALL) == -1) {
        //DEBUG("PEC Error!");
    }
}

void CMUControl::doTempConversion() {
//    return;
    DEBUG("Temperature conversion!");
	for(uint8_t i=0; i<6; ++i) { 	//i is channel number of MUX. Actual cell number need add +1
        DEBUG("  Channel %hhu", i);
        adc_mux(i);
        wait_ms(10); // XXX: Query Norio whether this is really meant to be 1000ms
		wakeup_sleep(); //To fix top LTC6804 keeps missing ADAX command.
        adax(); //Start GPIOs (General Purpose Input and Output) ADC Conversion and Poll Status
        wait_ms(7); // REFUP 4.4mS + Conversion 2.4mS
        if(rdaux(0, aux_codes) == -1)
            DEBUG("PEC error!");
        for(int j=0; j<NUM_CMUs; ++j) {
 //           DEBUG("    CMU %i Cell:%i,%i", j, i+1, i+7);
			temp_scaled[j][i] = TempScaling(aux_codes[j][0]);
			temp_scaled[j][i+6] = TempScaling(aux_codes[j][1]);
            DEBUG("    CMU %i Cell:%i,%i = %i, %i", j, i+1, i+7,temp_scaled[j][i],temp_scaled[j][i+6]);			
            DEBUG_ARRAY("      Raw temp data", "%hu", aux_codes[j], 2);

			}
    }
    return;
}


//       constexpr uint16_t THERMISTOR_CONST[10] = {22951,19956,16660,13387,10431,7951,5981,4475,3348,2515}; 
//	                                             //     0    10    20    30    40    50   60   70   80   90
//		VISHAY 10K 3977K
 
uint8_t CMUControl::TempScaling(uint16_t v_reading) {
	unsigned char i;
	unsigned char temp = 0;
	if (v_reading > THERMISTOR_CONST[0]) return(255);
	if (v_reading < THERMISTOR_CONST[9]) return(127);
	for(i = 1; i<9; i++){
		if(v_reading > THERMISTOR_CONST[i]) break;
		temp += 10;
	}
	v_reading -= THERMISTOR_CONST[i]; //1340
	temp += 10 - (uint8_t)(v_reading * 10 /(THERMISTOR_CONST[i-1] - THERMISTOR_CONST[i])); //3296
	return(temp);
}



void CMUControl::wrcfg(uint8_t config[][6]) {
    const uint8_t BYTES_IN_REG = 6;
    const uint8_t CMD_LEN = 4+(8*NUM_CMUs);
    uint8_t *cmd;
    uint16_t cfg_pec;
    uint8_t cmd_index; //command counter

    cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));

    //1
    cmd[0] = 0x00;
    cmd[1] = 0x01;
    cmd[2] = 0x3d;
    cmd[3] = 0x6e;

    //2
    cmd_index = 4;
    for (uint8_t current_ic = NUM_CMUs; current_ic > 0; current_ic--) {
        // executes for each LTC6804 in daisy chain, this loops starts with  - (THE CMUS ARE IN DAISY CHAIN DESIGN)
        // the last IC on the stack. The first configuration written is
        // received by the last IC in the daisy chain

        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) {
            // executes for each of the 6 bytes in the CFGR register
            // current_byte is the byte counter

            cmd[cmd_index] = config[current_ic-1][current_byte]; //adding the config data to the array to be sent
            cmd_index = cmd_index + 1;
        }
        //3
        cfg_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[current_ic-1][0]);
        cmd[cmd_index] = (uint8_t)(cfg_pec >> 8);
        cmd[cmd_index + 1] = (uint8_t)cfg_pec;
        cmd_index = cmd_index + 2;
    }

    //4
    wakeup_idle ();
    //5
    clearCS();
    wait_us(SPI_CS_DELAY); // SPI - Serial Peripheral Interface.
    spi_write_array(CMD_LEN, cmd);
    wait_us(SPI_CS_DELAY);
    setCS();
    free(cmd);
}


void CMUControl::set_adc(uint8_t MD, uint8_t DCP, uint8_t CH, uint8_t CHG) {
    uint8_t md_bits;

    md_bits = (MD & 0x02) >> 1;
    ADCV[0] = md_bits + 0x02;
    md_bits = (MD & 0x01) << 7;
    ADCV[1] =  md_bits + 0x60 + (DCP<<4) + CH;

    md_bits = (MD & 0x02) >> 1;
    ADAX[0] = md_bits + 0x04;
    md_bits = (MD & 0x01) << 7;
    ADAX[1] = md_bits + 0x60 + CHG;
}

/* LTC6804_adcv Function sequence:
 *
 * 1. Load adcv command into cmd array
 * 2. Calculate adcv cmd PEC and load pec into cmd array
 * 3. wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
 * 4. send broadcast adcv command to LTC6804 daisy chain
 */
void CMUControl::adcv() {
    uint8_t cmd[4];
    uint16_t cmd_pec;

    // 1
    cmd[0] = ADCV[0];
    cmd[1] = ADCV[1];

    // 2
    cmd_pec = pec15_calc(2, ADCV);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);

    // 3
    wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.

    // 4
    clearCS();
    wait_us(SPI_CS_DELAY);
    spi_write_array(4,cmd);
    wait_us(SPI_CS_DELAY);
    setCS();

}

void CMUControl::doCellBalance() {
	uint16_t balance_command;
    for(int cmuc=0; cmuc < Config::NUM_CMUs; ++cmuc) {
		balance_command = 0;
        for(int cell=0; cell < 12; cell++) {
			if(cell_codes[cmuc][cell]/10 > Config::CELL_BALANCE_VOLTAGE){
				balance_command |= (1 << cell);
			}	
		}
//        DEBUG("Set balance command %X",balance_command);
        tx_cfg[cmuc][0] = 0xFD; // GPIO[1:5], REFON, SWTEN, ADC.  Was 0xFE (0xFC for Norio)
        tx_cfg[cmuc][1] = 0x00; // Under voltage threshold
        tx_cfg[cmuc][2] = 0x00; // Under/over voltage threshold
        tx_cfg[cmuc][3] = 0x00; // Over voltage threshold
        tx_cfg[cmuc][4] = (uint8_t)balance_command & 0xff;
        tx_cfg[cmuc][5] = (uint8_t)(0x0 + ((balance_command >> 8) & 0x0f));	
	}
    wakeup_sleep();
    wrcfg(tx_cfg);
    return;
}




/* LTC6804_adax Function sequence:
 *
 * 1. Load adax command into cmd array
 * 2. Calculate adax cmd PEC and load pec into cmd array
 * 3. Wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
 * 4. Send broadcast adax command to LTC6804 daisy chain
 */
void CMUControl::adax() {
    uint8_t cmd[4];
    uint16_t cmd_pec;

    cmd[0] = ADAX[0];
    cmd[1] = ADAX[1];
    cmd_pec = pec15_calc(2, ADAX);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);

    wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.
    clearCS();
    wait_us(SPI_CS_DELAY);
    spi_write_array(4,cmd);
    wait_us(SPI_CS_DELAY);
    setCS();
}

/* LTC6804_rdcv Sequence
 *
 * 1. Switch Statement:
 *  a. Reg = 0
 *   i. Read cell voltage registers A-D for every IC in the daisy chain
 *   ii. Parse raw cell voltage data in cell_codes array
 *   iii. Check the PEC of the data read back vs the calculated PEC for each read register command
 *  b. Reg != 0
 *   i.Read single cell voltage register for all ICs in daisy chain
 *   ii. Parse raw cell voltage data in cell_codes array
 *   iii. Check the PEC of the data read back vs the calculated PEC for each read register command
 * 2. Return pec_error flag
 */
int8_t CMUControl::rdcv(uint8_t reg) {
    const uint8_t NUM_RX_BYT = 8;
    const uint8_t BYT_IN_REG = 6;
    const uint8_t CELL_IN_REG = 3;

    uint8_t *cell_data;
    uint8_t pec_error = 0;
    uint16_t parsed_cell;
    uint16_t received_pec;
    uint16_t data_pec;
    uint8_t data_counter=0; //data counter
    cell_data = (uint8_t *) malloc((NUM_RX_BYT * NUM_CMUs)*sizeof(uint8_t));
    //1.a
    if (reg == 0) {
        //a.i
        // For each cell voltage register
        //rdcv_reg(1, cell_data);
        wakeup_sleep();
        wait_us(10);
        for (uint8_t cell_reg = 1; cell_reg<5; cell_reg++) {
            data_counter = 0;
            // Read one register
            rdcv_reg(cell_reg, cell_data);

            //DEBUG_ARRAY("Raw cell data", "%02X", cell_data, NUM_RX_BYT * NUM_CMUs);

            // For each IC in the chain
            for (uint8_t current_ic = 0 ; current_ic < NUM_CMUs; current_ic++) {
                //a.ii
                // Parse each of the 3 cell voltage codes in the register
                for (uint8_t current_cell = 0; current_cell<CELL_IN_REG; current_cell++) {
                    // Each cell code is received as 2 bytes and combined
                    parsed_cell = cell_data[data_counter] + (cell_data[data_counter + 1] << 8);

                    cell_codes[current_ic][current_cell  + ((cell_reg - 1) * CELL_IN_REG)] = parsed_cell;

                    data_counter += 2; // 2 bytes per cell code
                }
                //a.iii
                // The received PEC for the current_ic is transmitted as the 7th and 8th
                //  after the 6 cell voltage data bytes
                received_pec = (cell_data[data_counter] << 8) + cell_data[data_counter+1];
                data_pec = pec15_calc(BYT_IN_REG, &cell_data[current_ic * NUM_RX_BYT]);
                if (received_pec != data_pec) {
                    //DEBUG("PEC error: %hu != %hu", received_pec, data_pec);
                    pec_error = -1;
                }
                data_counter += 2; // 2 bytes of PEC data
            }
        }
    } else { // 1.b
        //b.i
        rdcv_reg(reg, cell_data);
        for (uint8_t current_ic = 0 ; current_ic < NUM_CMUs; current_ic++) {
            //b.ii
            // Parse each of the 3 cell voltage codes in the register
            for (uint8_t current_cell = 0; current_cell < CELL_IN_REG; current_cell++) {
                // Each cell code is received as 2 bytes and combined
                parsed_cell = cell_data[data_counter] + (cell_data[data_counter+1]<<8);

                cell_codes[current_ic][current_cell + ((reg - 1) * CELL_IN_REG)] = 0x0000FFFF & parsed_cell;
                data_counter += 2; // 2 bytes per cell code
            }
            //b.iii
            // The received PEC for the current_ic is transmitted as the 7th and 8th
            //  after the 6 cell voltage data bytes
            received_pec = (cell_data[data_counter] << 8 )+ cell_data[data_counter + 1];
            data_pec = pec15_calc(BYT_IN_REG, &cell_data[current_ic * NUM_RX_BYT]);
            if (received_pec != data_pec) {
                pec_error = -1;
            }
            data_counter += 2;
        }
    }

    //2
    free(cell_data);
    return pec_error;
}

/* LTC6804_rdcv_reg Function Process:
 * 1. Determine Command and initialize command array
 * 2. Calculate Command PEC
 * 3. Wake up isoSPI, this step is optional
 * 4. Send Global Command to LTC6804 daisy chain
 */
void CMUControl::rdcv_reg(uint8_t reg, uint8_t *data) {
    const uint8_t REG_LEN = 8; //number of bytes in each ICs register + 2 bytes for the PEC
    uint8_t cmd[4];
    uint16_t cmd_pec;

    //1
    if (reg == 1) {   //1: RDCVA
        cmd[1] = 0x04;
        cmd[0] = 0x00;
    } else if (reg == 2) { //2: RDCVB
        cmd[1] = 0x06;
        cmd[0] = 0x00;
    } else if (reg == 3) { //3: RDCVC
        cmd[1] = 0x08;
        cmd[0] = 0x00;
    } else if (reg == 4) { //4: RDCVD
        cmd[1] = 0x0A;
        cmd[0] = 0x00;
    } else {
        ERROR("Invalid register");
        return;
    }

    //2
    cmd_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);

    //3
    wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.

    //4
    clearCS();
    wait_us(SPI_CS_DELAY);
    spi_write_read(cmd,4,data,(REG_LEN*NUM_CMUs));
    wait_us(SPI_CS_DELAY);
    setCS();
}

/* LTC6804_rdaux Sequence

   1. Switch Statement:
   a. Reg = 0
   i. Read GPIO voltage registers A-D for every IC in the daisy chain
   ii. Parse raw GPIO voltage data in cell_codes array
   iii. Check the PEC of the data read back vs the calculated PEC for each read register command
   b. Reg != 0
   i.Read single GPIO voltage register for all ICs in daisy chain
   ii. Parse raw GPIO voltage data in cell_codes array
   iii. Check the PEC of the data read back vs the calculated PEC for each read register command
   2. Return pec_error flag
   */
int8_t CMUControl::rdaux(uint8_t reg, uint16_t aux_codes[][6]) {
    const uint8_t NUM_RX_BYT = 8;
    const uint8_t BYT_IN_REG = 6;
    const uint8_t GPIO_IN_REG = 3;

    uint8_t *data;
    uint8_t data_counter = 0;
    int8_t pec_error = 0;
    uint16_t parsed_aux;
    uint16_t received_pec;
    uint16_t data_pec;
    data = (uint8_t *) malloc((NUM_RX_BYT*NUM_CMUs)*sizeof(uint8_t));
    //1.a
    
    wakeup_sleep();

    if (reg == 0) {
        //a.i
        for (uint8_t gpio_reg = 1; gpio_reg<3; gpio_reg++) {
            //executes once for each of the LTC6804 aux voltage registers

            data_counter = 0;
            rdaux_reg(gpio_reg,data); //Reads the raw auxiliary register data into the data[] array

            //DEBUG_ARRAY("Raw temp data", "%02X", data, NUM_RX_BYT * NUM_CMUs);

            for (uint8_t current_ic = 0 ; current_ic < NUM_CMUs; current_ic++) {
                //a.ii
                for (uint8_t current_gpio = 0; current_gpio< GPIO_IN_REG; current_gpio++) {
                    // This loop parses the read back data into GPIO voltages, it
                    //  loops once for each of the 3 gpio voltage codes in the register

                    parsed_aux = data[data_counter] + (data[data_counter+1]<<8);

                    aux_codes[current_ic][current_gpio +((gpio_reg-1)*GPIO_IN_REG)] = parsed_aux;
                    data_counter=data_counter+2; // Because gpio voltage codes are two bytes the data counter
                    // must increment by two for each parsed gpio voltage code

                }

                //a.iii
                received_pec = (data[data_counter]<<8)+ data[data_counter+1];
                //The received PEC for the current_ic is transmitted as the 7th and 8th
                //after the 6 gpio voltage data bytes
                data_pec = pec15_calc(BYT_IN_REG, &data[current_ic*NUM_RX_BYT]);
                if (received_pec != data_pec) {
                    pec_error = -1;
                }

                data_counter=data_counter+2; // Because the transmitted PEC code is 2 bytes long the data_counter
                // must be incremented by 2 bytes to point to the next ICs gpio voltage data
            }
        }
    } else {
        //b.i
        rdaux_reg(reg, data);
        for (int current_ic = 0 ; current_ic < NUM_CMUs; current_ic++) {

            //b.ii
            for (int current_gpio = 0; current_gpio<GPIO_IN_REG; current_gpio++) {
                // This loop parses the read back data. Loops
                // once for each aux voltage in the register

                parsed_aux = (data[data_counter] + (data[data_counter+1]<<8));
                aux_codes[current_ic][current_gpio +((reg-1)*GPIO_IN_REG)] = parsed_aux;
                data_counter=data_counter+2; // Because gpio voltage codes are two bytes the data counter
                // must increment by two for each parsed gpio voltage code
            }
            //b.iii
            received_pec = (data[data_counter]<<8) + data[data_counter+1];
            // The received PEC for the current_ic is transmitted as the 7th and 8th
            // after the 6 gpio voltage data bytes
            data_pec = pec15_calc(BYT_IN_REG, &data[current_ic*NUM_RX_BYT]);
            if (received_pec != data_pec) {
                pec_error = -1; //The pec_error variable is simply set negative if any PEC errors
                //are detected in the received serial data
            }

            data_counter=data_counter+2; // Because the transmitted PEC code is 2 bytes long the data_counter
            // must be incremented by 2 bytes to point to the next ICs gpio voltage data
        }
    }
    free(data);
    return (pec_error);
}

/*
   LTC6804_rdaux_reg Function Process:
   1. Determine Command and initialize command array
   2. Calculate Command PEC
   3. Wake up isoSPI, this step is optional
   4. Send Global Command to LTC6804 daisy chain
   */
void CMUControl::rdaux_reg(uint8_t reg, uint8_t *data) {
    const uint8_t REG_LEN = 8; // number of bytes in the register + 2 bytes for the PEC
    uint8_t cmd[4];
    uint16_t cmd_pec;

    //1
    if (reg == 1) {   //Read back auxiliary group A
        cmd[1] = 0x0C;
        cmd[0] = 0x00;
    } else if (reg == 2) { //Read back auxiliary group B
        cmd[1] = 0x0e;
        cmd[0] = 0x00;
    } else {      //Read back auxiliary group A
        cmd[1] = 0x0C;
        cmd[0] = 0x00;
    }
    //2
    cmd_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);

    //3
    wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake, this command can be removed.
    //4
    clearCS();
    wait_us(SPI_CS_DELAY);
    spi_write_read(cmd,4,data,(REG_LEN*NUM_CMUs));
    wait_us(SPI_CS_DELAY);
    setCS();
}

void CMUControl::adc_mux(uint8_t channel) {
	///this "channel is output port number of MUX IC (6 chanels for 0 to 5)
    uint8_t com_codes[NUM_CMUs][6];
	for(int cmuc=0; cmuc < Config::NUM_CMUs; ++cmuc) {
		com_codes[cmuc][0]= (I2CSTART << 4) + 0x09;
		com_codes[cmuc][1]= (0x0 << 4) + I2CNACK;
		com_codes[cmuc][2]= (I2CBLANK << 4) + 0x0;
		com_codes[cmuc][3]= ((0x8 + ((channel) & 0x7)) << 4) + I2CNACKSTOP;
		com_codes[cmuc][4]= (I2CNOTRANSMIT << 4) + 0x0;
		com_codes[cmuc][5]= 0x00;
	}
    wakeup_sleep();
    wait_us(10);

    DEBUG("Set Mux Reg to %d\r\n",channel);
    wrcom(com_codes);
	wait_us(1);
    excom();
}

void CMUControl::wrcom(uint8_t config[][6]) {
    constexpr uint8_t BYTES_IN_REG = 6;
    constexpr uint8_t CMD_LEN = 4+(8*NUM_CMUs);
    uint8_t cmd[CMD_LEN];
    uint16_t cfg_pec;
    uint8_t cmd_index; //command counter

    //1
    cmd[0] = 0x07;
    cmd[1] = 0x21;
    cmd[2] = 0x24;
    cmd[3] = 0xb2;

    //2
    cmd_index = 4;
    for (uint8_t current_ic = NUM_CMUs; current_ic > 0; current_ic--) {
        // Executes for each LTC6804 in daisy chain, this loops starts with
        // the last IC on the stack. The first command written is
        // received by the last IC in the daisy chain

        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) {
            // current_byte is the byte counter

            cmd[cmd_index] = config[current_ic-1][current_byte]; // Adding the config data to the array to be sent
            cmd_index = cmd_index + 1;
        }

        //3
        cfg_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[current_ic-1][0]);
        cmd[cmd_index] = (uint8_t)(cfg_pec >> 8);
        cmd[cmd_index + 1] = (uint8_t)cfg_pec;
        cmd_index = cmd_index + 2;
    }

    //4
    wakeup_idle ();

    //5
    clearCS();
    wait_us(SPI_CS_DELAY);
    spi_write_array(CMD_LEN, cmd);
    wait_us(SPI_CS_DELAY);
    setCS();
}

void CMUControl::excom() {
    uint8_t cmd[4];
    uint16_t cmd_pec;
    int loop;

    //1
    cmd[0] = 0x07;
    cmd[1] = 0x23;

    //2
    cmd_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);

    //3
    wakeup_idle(); //This will guarantee that the LTC6804 isoSPI port is awake.This command can be removed.
    //4
    clearCS();
    wait_us(SPI_CS_DELAY);
    spi_write_array(4, cmd);

    for (loop = 0; loop<72; loop++) {
        spi.write(0xff);
    }

    wait_us(SPI_CS_DELAY);

    setCS();
}


void CMUControl::wakeup_sleep() {
    clearCS();
    wait_us(300); // Guarantees the LTC6804 won't be sleeping
    setCS();
}


//// tReady should be longer than 10uS for large stack.
void CMUControl::wakeup_idle() {
    clearCS();
    wait_us(10); // Guarantees the isoSPI interface won't be sleeping
    setCS();
}

uint16_t CMUControl::pec15_calc(uint8_t len, const uint8_t *data) {
    uint16_t remainder = 16;
    uint16_t addr;

    for (uint8_t i = 0; i<len; i++) {
        addr = ((remainder>>7)^data[i]) & 0xff;//calculate PEC table address
        remainder = (remainder<<8)^CRC15TABLE[addr];
    }
    return remainder*2; // The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}

void CMUControl::spi_write_array(uint8_t len, const uint8_t *data) {
    for (uint8_t i = 0; i < len; i++) {
        // wait_us(1);
        spi.write((int8_t)data[i]);
        wait_us(3);
        //  cs = 1;
    }
}

void CMUControl::spi_write_read(const uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t rx_len) {
    for (uint8_t i = 0; i < tx_len; i++) {
        spi.write(tx_data[i]);
        wait_us(3);
    }

    for (uint8_t i = 0; i < rx_len; i++) {
        rx_data[i] = (uint8_t)spi.write(0xff);
        wait_us(3);
    }
}

