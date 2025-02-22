#include <u.h>
#include <libc.h>

#define PIO 21

/* ROM Commands */
#define ROM_READ 0x33
#define ROM_MATCH 0x55
#define ROM_SEARCH 0xF0
#define ROM_SKIP 0xCC
#define ROM_RESUME 0xA5
#define ROM_ODSKIP 0x3C
#define ROM_ODMATCH 0x69

/* Memory Function Commands */
#define WS 0x0F
#define RS 0xAA
#define CPS 0x55
#define RM 0xF0

void w1wire(int, uchar);
uchar r1wire(int);
int reset1wire(int);
void udelay(long);

int fd_gpio;

void main(){
	int i;
	uchar d[8] = {0};
	ulong id;

	fd_gpio = open("/dev/gpio", MAFTER);
	if(fd_gpio < 0){
		bind("#G", "/dev", MAFTER);
		fd_gpio = open("/dev/gpio", MAFTER);
		if(fd_gpio < 0){
			fprint(2, "ERROR: Failed to open GPIO driver file\n");
			return;
		}
	}
	fprint(fd_gpio, "function %d in", PIO);
	fprint(fd_gpio, "set %d 0", PIO);

	while(1){
		// Get ID
		if(reset1wire(fd_gpio) == 0){
			fprint(2, "1 wire reset failure\n");
			return;
		}
		w1wire(fd_gpio, ROM_READ);
		for(i = 0; i < 8; i++){
			d[i] = r1wire(fd_gpio);
		}
		id  = d[1] << 40;
		id |= d[2] << 32;
		id |= d[3] << 24;
		id |= d[4] << 16;
		id |= d[5] << 8;
		id |= d[6];
		print("Family: 0x%ux\n", d[0]);
		print("ID 0x%lux\n", id);

		print("");
		sleep(1000);
	}
}

void w1wire(int fd, uchar c){
	int i;
	for(i = 0; i < 8; i++){
		if(c & 0x01){
			fprint(fd, "function %d pulse", PIO);
			udelay(100);
		}
		else{
			fprint(fd, "function %d out", PIO);
			udelay(60);
			fprint(fd, "function %d in", PIO);
			udelay(40);
		}
		c >>= 1;
	}	
}

uchar r1wire(int fd){
	uvlong x;
	int i;
	uchar c;
	char buf[17];

	c = 0;
	for(i = 0; i < 8; ++i){
		c >>= 1;
		fprint(fd, "function %d pulse", PIO);
		if(read(fd, buf, 16) < 0){
			fprint(2, "Read error %r\n");
			return 0;
		}
		buf[16] = 0;
		x = strtoull(buf, nil, 16);
		if(x & (1 << PIO)){
			c |= 0x80;
		}
		udelay(20);
	}
	return c;
}

int reset1wire(int fd){
	uvlong x;
	char buf[17];
	fprint(fd, "function %d out", PIO);
	udelay(600);
	fprint(fd, "function %d in", PIO);
	udelay(60);
	if(read(fd, buf, 16) < 0){
		fprint(2, "Read error %r\n");
		return 0;
	}
	buf[16] = 0;
	x = strtoull(buf, nil, 16);
	udelay(100);
	if(x & (1 << PIO))
		return 0;
	return 1;
}

void udelay(long t){
	ulong i;
	t *= 1000;
	for(i = 0; i < t; i++) {}
}