#include <u.h>
#include <libc.h>

/* 1-Wire IO Pin Connction */
#define PIO 21

/* Timing macros */
#define TRSTL 600
#define TPDH 60
#define TRSTH 480
#define TSLOT 100
#define TW0L 60
#define TPRG 12

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

/* Other magic numbers */
#define CPS_OK 0xAA
#define ADDR_MAX 0xFF

int read_mem(int, uchar *, ushort, uint);
int write_mem(int, uchar *, ushort);
uchar copy_sc(int, ushort, uchar);
ushort read_sc(int, uchar *, ushort *, uchar *);
ushort write_sc(int, uchar *, ushort);
ulong get_id(int, uchar *, uchar *);
void w1wire(int, uchar);
uchar r1wire(int);
int reset1wire(int);
void udelay(long);

void main(){
	int i;
	int fd_gpio;
	uchar d[8] = {0};
	ushort addr;
	uchar ret;
	ulong id;
	uchar fam;

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

	print("~~~~~~~~~~ 1-Wire Project ~~~~~~~~~~\n");

	id = get_id(fd_gpio, &fam, nil);
	print("Product: Family(0x%x), ID(0x012x)\n", fam, id);

	sleep(10);

	read_mem(fd_gpio, d, 0x0085, 1);
	print("Factory Byte: 0x%x\n", d[0]);

	sleep(10);

	for(i = 0; i  < 8; i++) d[i] = 8 - i;
	addr = 0x0000;
	write_mem(fd_gpio, d, addr);

	sleep(500);
	for(i = 0; i < 8; i++) d[i] = 0;

	read_mem(fd_gpio, d, addr, 8);
	for(i = 0; i < 8; i++) print("Mem read %d: %d\n", i, d[i]);
}

int read_mem(int fd, uchar *d, ushort address, uint nbytes){
	int i;
	int count;
	if(!d){
		fprint(2, "ERROR: read_mem received bad data buf\n");
		return -1;
	}
	if(address >= ADDR_MAX){
		fprint(2, "ERROR: invalid memory address to read from\n");
		return -1;
	}
	if(reset1wire(fd) == 0){
		fprint(2, "1wire reset failure\n");
		return -1;
	}
	w1wire(fd, ROM_SKIP);
	w1wire(fd, RM);
	w1wire(fd, address & 0xFF);
	w1wire(fd, address >> 8);
	for(count = 0; count < nbytes; count++){
		if(address + count >= ADDR_MAX) break;
		d[count] = r1wire(fd);
	}
	return count;	
}

int write_mem(int fd, uchar *d, ushort address){
	ushort crc16;
	ushort TA;
	uchar ES;
	uchar ret;

	if(!d){
		fprint(2, "ERROR: write_mem received bad data buf\n");
		return -1;
	}

	TA = address;
	crc16 = write_sc(fd, d, TA);
	print("Wrote to SC, TA: 0x%04x (CRC16: 0x%04x)\n", TA, crc16);
	
	crc16 = read_sc(fd, d, &TA, &ES);
	print("Read from SC, TA: 0x%04x, ES:0x%x (CRC16: 0x%04x)\n", TA, ES, crc16);

	ret = 0 | copy_sc(fd, TA, ES);
	print("Copied SC to MEM: ret: 0x%x\n", ret);

	return ret != CPS_OK;
}

uchar copy_sc(int fd, ushort TA, uchar ES){
	uchar ret;

	if(reset1wire(fd) == 0){
		fprint(2, "1wire reset failure\n");
		return 0xFF;
	}
	w1wire(fd, ROM_SKIP);
	w1wire(fd, CPS);
	w1wire(fd, TA & 0xFF);
	w1wire(fd, TA >> 8);
	w1wire(fd, ES);
	ret = r1wire(fd);

	return ret;
}

ushort read_sc(int fd, uchar *d, ushort *TA_p, uchar *ES_p){
	uchar i;
	ushort TA;
	uchar ES;
	uchar b2rd;
	ushort crc16;

	if(!d){
		fprint(2, "ERROR: read_sc received invalid data buffer\n");
		return 0;
	}
	if(reset1wire(fd) == 0){
		fprint(2, "1 wire reset failure\n");
		return;
	}
	w1wire(fd, ROM_SKIP);
	w1wire(fd, RS);
	TA = r1wire(fd);
	TA |= r1wire(fd) << 8;
	ES = r1wire(fd);
	b2rd = (ES & 0x7) - (TA & 0x7) + 1;
	for(i = 0; i < b2rd; i++) d[i] = r1wire(fd);
	crc16 = r1wire(fd);
	crc16 |= r1wire(fd) << 8;

	if(TA_p) *TA_p = TA;
	if(ES_p) *ES_p = ES;

	return crc16;
}

ushort write_sc(int fd, uchar *d, ushort TA){
	int i;
	ushort crc16;
	if(!d){
		fprint(2, "ERROR: write_sc received invalid data buffer\n");
		return 0;
	}
	if(reset1wire(fd) == 0){
		fprint(2, "1 wire reset failure\n");
		return 0;
	}
	w1wire(fd, ROM_SKIP);
	w1wire(fd, WS);
	w1wire(fd, TA & 0xFF);
	w1wire(fd, TA >> 8);
	
	for(i = 0; i < 8; i++) w1wire(fd, d[i]);
	crc16  = r1wire(fd);
	crc16 |= r1wire(fd) << 8;

	return crc16;
}

ulong get_id(int fd, uchar *fam_p, uchar *crc8_p){
	int i;
	ulong id;
	uchar fam;
	uchar crc8;
	uchar d[8] = {0};

	if(reset1wire(fd) == 0){
		fprint(2, "1 wire reset failure\n");
		return;
	}
	w1wire(fd, ROM_READ);
	for(i = 0; i < 8; i++) d[i] = r1wire(fd);
	fam = d[0];
	id  = d[1] << 40;
	id |= d[2] << 32;
	id |= d[3] << 24;
	id |= d[4] << 16;
	id |= d[5] << 8;
	id |= d[6];
	crc8 = d[7];

	if(fam_p) *fam_p = fam;
	if(crc8_p) *crc8_p = crc8;

	return id;
}

void w1wire(int fd, uchar c){
	int i;
	for(i = 0; i < 8; i++){
		if(c & 0x01){
			fprint(fd, "function %d pulse", PIO);
			udelay(TSLOT);
		}
		else{
			fprint(fd, "function %d out", PIO);
			udelay(TW0L);
			fprint(fd, "function %d in", PIO);
			udelay(TSLOT - TW0L);
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
	udelay(TRSTL);
	fprint(fd, "function %d in", PIO);
	udelay(TPDH);
	if(read(fd, buf, 16) < 0){
		fprint(2, "Read error %r\n");
		return 0;
	}
	buf[16] = 0;
	x = strtoull(buf, nil, 16);
	udelay(TRSTH);
	if(x & (1 << PIO))
		return 0;
	return 1;
}

void udelay(long t){
	ulong i;
	t *= 1000;
	for(i = 0; i < t; i++) {}
}
