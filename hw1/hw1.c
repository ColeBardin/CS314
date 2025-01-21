#include <u.h>
#include <libc.h>

#define BUTTON 20 /* Pin 20 - Button Input */
#define LED 21 /* Pin 21 - LED Output */

typedef enum{
	LED_OFF,
	LED_ON,
	LED_FLASH
} led_state_e;

int read_pin(int gfd, int pin);

void main(){
	int fd;
	int button;
	int led_flash = 0;
	led_state_e state = LED_OFF;

	bind("#G", "/dev", MAFTER);
	fd = open("/dev/gpio", ORDWR);
	if(fd < 0){
		fprint(2, "ERROR: Failed to open GPIO driver file\n");
		return;
	}
	fprint(fd, "function %d in", BUTTON);
	fprint(fd, "pulldown %d", BUTTON);
	fprint(fd, "function %d out", LED);
	fprint(fd, "pulldown %d", LED);

	for(;;){
		button = read_pin(fd, BUTTON);
		if(button){
			while(read_pin(fd, BUTTON)) sleep(10);
			state++;
			if(state > LED_FLASH) state = LED_OFF;
		}

		switch(state){
			case LED_OFF:
				fprint(fd, "set %d 0", LED);
				break;
			case LED_ON:
				fprint(fd, "set %d 1", LED);	
				break;
			case LED_FLASH:
				fprint(fd, "set %d %d", LED, led_flash);
				led_flash = !led_flash;
				break;
		}
		sleep(100);
	}		
}

int read_pin(int gfd, int pin){
	char buf[17] = {0};
	uvlong gvals;

	read(gfd, buf, 16);
	gvals = strtoull(buf, nil, 16);

	return 1 & (gvals >> pin);
}