#include "LTC6813.h"

#define NSEGS 1
#define CSA 8
#define CSB 7

void main(){
	cell_asic ic_arr[NSEGS];
	ltc6813_driver_t ltc;

	LTC6813_init(&ltc, CSA, CSB, NSEGS, ic_arr);
	return;
}

