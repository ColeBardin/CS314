#include "LTC6813.h"

/*
	LTC6820 SPI to isoSPI transceiver
		isoSPI is a 2 wire differential pair translation of SPI, typically named IPA and IMA
		Devices can be daisy-chained
		The last device can optionally be connected back to the first to create a loop of 2 directional strings
		This String B adds IPB and IMB
		This allows for bidirectional communication that can compensate for a break in the chain
		ASIC allows for isolation communication to different voltage systems
		Protocol comprises of pulses. Pulses have a duration and a sign
		There are long pulses and short pulses measured Vipa to Vima
		At rest Vipa = Vima
		+1 pulses consist of Vipa > Vima then Vipa < Vima then stabilizng
		-1 pulses consist of Vipa < Vima then Vipa > Vima then stabilizing
		The duration of high and low segments determine if a pulse is long or short.
		Long +1: CS Low
		Long -1: CS High
		Short +1: Data 1 and clock pulse
		Short -1: Data 0 and clock pulse


	LTC6813 Multi-cell battery stack monitor
		Battery Management IC (BMIC)
		Measures up to 18 individual battery cell voltages
		9 Auxiliary GPIO & Analog inputs
		Supports individual passive cell balancing control
		Can act as an SPI or I2C master
		Natively speaks isoSPI
		Supports bidirectional daisy-chained isoSPI architecture
		When it receives duplicate commands on IPA/IMA, forwards them to IPB/IMB
		Waits for and forwards responses from other devices IPB/IMB and forwards them on IPA/IMA

		This chip is really cool and the datasheet is worth a read or five. 
		Their new generation version ADBMS6830 is even cooler

	System Architecture:
	RPi <-(SPI)-> String A LTC6820 <-(isoSPI)-> Segment 0 LTC6820 <-(isoSPI)-> Segment 1 LTC6820 <-(isoSPI)-> ... <-(isoSPI)-> String B LTC6820 <-(SPI)-> RPi
*/

#define NSEGS 1
#define NCELLS 14

#define CSA 20
#define CSB 16

typedef struct
{
	float total_volt;
	float max_volt;
	float min_volt;
	float max_temp;
    	cell_asic ic_arr[NSEGS];
    	ltc6813_driver_t ltc;
    	ltc681x_conf_t cfg;
    	int num_ics;
} accumulator_t;

void accumulator_init(accumulator_t *acc);
int accumulator_read_volt(accumulator_t *acc);
int accumulator_convert_volt(accumulator_t *acc);
void accumulator_print_stat(accumulator_t *acc);

void main(){
    	accumulator_t acc;

    	accumulator_init(&acc);

    	while(1){
        		accumulator_read_volt(&acc);
		accumulator_print_stat(&acc);
        		sleep(5000);
    	}

    	return;
}

void accumulator_init(accumulator_t *acc){
	if(!acc) return;
    	int i;
	ltc6813_driver_t *ltc = &acc->ltc;
	if(!ltc) return;

	acc->cfg.OV_THRESHOLD = 0;

	acc->cfg.REFON = 1; //!< Reference Powered Up Bit
	acc->cfg.ADCOPT = 0; //!< ADC Mode option bit
	//!< GPIO Pin Control // Gpio 1,2,3,4,5
	for(int i = 0; i < 5; i++) acc->cfg.GPIOBITS_A[i] = 1;
	//!< GPIO Pin Control // Gpio 6,7,8,9
	for(int i = 0; i < 4; i++) acc->cfg.GPIOBITS_B[i] = 1;
	acc->cfg.UV=acc->cfg.UV_THRESHOLD; //!< Under voltage Comparison Voltage
	acc->cfg.OV=acc->cfg.OV_THRESHOLD; //!< Over voltage Comparison Voltage
	//!< Discharge cell switch //Dcc 1,2,3,4,5,6,7,8,9,10,11,12
	for(int i = 0; i < 12; i++) acc->cfg.DCCBITS_A[i] = 0;
	//!< Discharge cell switch //Dcc 0,13,14,15
	for(int i = 0; i < 7; i++) acc->cfg.DCCBITS_B[i] = 0;
	//!< Discharge time value //Dcto 0,1,2,3  // Programed for 4 min
	acc->cfg.DCTOBITS[0] = 1;
	acc->cfg.DCTOBITS[1] = 0;
	acc->cfg.DCTOBITS[2] = 1;
	acc->cfg.DCTOBITS[3] = 0;
	/*Ensure that Dcto bits are set according to the required discharge time. Refer to the data sheet */
	acc->cfg.FDRF = 0; //!< Force Digital Redundancy Failure Bit
	acc->cfg.DTMEN = 1; //!< Enable Discharge Timer Monitor
	//!< Digital Redundancy Path Selection//ps-0,1
	acc->cfg.PSBITS[0]= 0;
	acc->cfg.PSBITS[1]= 0;

	LTC6813_init(ltc, CSA, CSB, NSEGS, acc->ic_arr);

	wakeup_sleep(ltc);
	LTC6813_init_cfg(ltc);
	LTC6813_init_cfgb(ltc);

	for(i = 0; i < NSEGS; i++){
		LTC6813_set_cfgr(ltc,
	    				 i,
						 acc->cfg.REFON,
						 acc->cfg.ADCOPT,
						 acc->cfg.GPIOBITS_A,
						 acc->cfg.DCCBITS_A,
						 acc->cfg.DCTOBITS,
						 acc->cfg.UV,
						 acc->cfg.OV);
	    LTC6813_set_cfgrb(ltc,
	    		  	  	  i,
						  acc->cfg.FDRF,
						  acc->cfg.DTMEN,
						  acc->cfg.PSBITS,
						  acc->cfg.GPIOBITS_B,
						  acc->cfg.DCCBITS_B);
	}
	LTC6813_wrcfg(ltc);
	LTC6813_wrcfgb(ltc);
	LTC6813_reset_crc_count(ltc);
	LTC6813_init_reg_limits(ltc);
}

int accumulator_read_volt(accumulator_t *acc){
	if(!acc) return -1;
	ltc6813_driver_t *ltc = &acc->ltc;
	if(!ltc) return -2;

	int ret = 0;
	int error = 0;
	uint32_t conv = 0;

	wakeup_sleep(ltc);
	LTC6813_mute(ltc);
	LTC6813_adcv(ltc, MD_7KHZ_3KHZ, DCP_DISABLED, CELL_CH_ALL);
	LTC6813_unmute(ltc);
	conv = LTC6813_pollAdc(ltc);
    wakeup_sleep(ltc);
    do{
    	error = LTC6813_rdcv(ltc, REG_ALL);
    } while(error == -1);
    ret |= accumulator_convert_volt(acc);
    return ret;
}

int accumulator_convert_volt(accumulator_t *acc){
	if(!acc) return -1;
	int seg, row;
	float total = 0;
	float max = -0.3, min = 21;
	float seg_total, seg_min, seg_max;
	float volt;

	for(seg = 0; seg < NSEGS; seg++)
	{
		seg_total = 0;
		seg_min = 21;
		seg_max = -0.3;
		for(row = 0; row < NCELLS; row++)
		{
			volt = (float)acc->ic_arr[seg].cells.c_codes[row] * 0.0001;
			acc->ic_arr[seg].voltage[row] = volt;
			total += volt;
			seg_total += volt;
			if(volt > seg_max) seg_max = volt;
			if(volt < seg_min) seg_min = volt;
			if(volt > max) max = volt;
			if(volt < min) min = volt;
		}
		acc->ic_arr[seg].min_volt = seg_min;
		acc->ic_arr[seg].max_volt = seg_max;
		acc->ic_arr[seg].total_volt = seg_total;
	}
	acc->min_volt = min;
	acc->max_volt = max;
	acc->total_volt = total;

	return 0;
}

void accumulator_print_stat(accumulator_t *acc){
    	if(!acc) return;

    	int seg;
	int cell;

	char *line = "~~~~~~~~~~~~~~~~~~~~\n";

	print(line);
    	print("Total Voltage: %5.2f V\n", acc->total_volt);
    	print("Max Voltage:   %5.2f V\n", acc->max_volt);
    	print("Min Voltage:   %5.2f V\n", acc->min_volt);
    	for(seg = 0; seg < NSEGS; seg++){
        		print("\tSegment %d\n", seg);
        		print("\tTotal Voltage: %5.2f V\n", acc->ic_arr[seg].total_volt);
        		print("\tMax Voltage:   %5.2f V\n", acc->ic_arr[seg].max_volt);
        		print("\tMin Voltage:   %5.2f V\n", acc->ic_arr[seg].min_volt);
		for(cell = 0; cell < NCELLS; cell++) print("[#%2d:  %4.2f V] ", cell, acc->ic_arr[seg].voltage[cell]);
        		print("\n\n");
    	}
	print(line);
}

