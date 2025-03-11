#include "LTC6813.h"

#define NSEGS 1
#define CSA 8
#define CSB 7

void main(){
    int i;
	cell_asic ic_arr[NSEGS];
	ltc6813_driver_t ltc;
    ltc681x_conf_t cfg;

	cfg.OV_THRESHOLD = 0;

	cfg.REFON = 1; //!< Reference Powered Up Bit
	cfg.ADCOPT = 0; //!< ADC Mode option bit
	//!< GPIO Pin Control // Gpio 1,2,3,4,5
	for(int i = 0; i < 5; i++) cfg.GPIOBITS_A[i] = 1;
	//!< GPIO Pin Control // Gpio 6,7,8,9
	for(int i = 0; i < 4; i++) cfg.GPIOBITS_B[i] = 1;
	cfg.UV=cfg.UV_THRESHOLD; //!< Under voltage Comparison Voltage
	cfg.OV=cfg.OV_THRESHOLD; //!< Over voltage Comparison Voltage
	//!< Discharge cell switch //Dcc 1,2,3,4,5,6,7,8,9,10,11,12
	for(int i = 0; i < 12; i++) cfg.DCCBITS_A[i] = 0;
	//!< Discharge cell switch //Dcc 0,13,14,15
	for(int i = 0; i < 7; i++) cfg.DCCBITS_B[i] = 0;
	//!< Discharge time value //Dcto 0,1,2,3  // Programed for 4 min
	cfg.DCTOBITS[0] = 1;
	cfg.DCTOBITS[1] = 0;
	cfg.DCTOBITS[2] = 1;
	cfg.DCTOBITS[3] = 0;
	/*Ensure that Dcto bits are set according to the required discharge time. Refer to the data sheet */
	cfg.FDRF = 0; //!< Force Digital Redundancy Failure Bit
	cfg.DTMEN = 1; //!< Enable Discharge Timer Monitor
	//!< Digital Redundancy Path Selection//ps-0,1
	cfg.PSBITS[0]= 0;
	cfg.PSBITS[1]= 0;

	LTC6813_init(&ltc, CSA, CSB, NSEGS, ic_arr);

	wakeup_sleep(&ltc);
	LTC6813_init_cfg(&ltc);
	LTC6813_init_cfgb(&ltc);

	for(i = 0; i < NSEGS; i++)
	{
	    LTC6813_set_cfgr(&ltc,
	    				 i,
						 cfg.REFON,
						 cfg.ADCOPT,
						 cfg.GPIOBITS_A,
						 cfg.DCCBITS_A,
						 cfg.DCTOBITS,
						 cfg.UV,
						 cfg.OV);
	    LTC6813_set_cfgrb(&ltc,
	    		  	  	  i,
						  cfg.FDRF,
						  cfg.DTMEN,
						  cfg.PSBITS,
						  cfg.GPIOBITS_B,
						  cfg.DCCBITS_B);
	}
	LTC6813_wrcfg(&ltc);
	LTC6813_wrcfgb(&ltc);
	LTC6813_reset_crc_count(&ltc);
	LTC6813_init_reg_limits(&ltc);

	return;
}

