#include <u.h>
#include <libc.h>

// Using ST7789V2 Chip on Breakout Board

#define WIDTH 240
#define HEIGHT 280

#define CS 8 /* Pin 8/CE0 is chip select for SPI client */
#define DC 21 /* Pin 16 is the Data Command output for the chip */
#define RST 20 /* Pin 20 is the RST pin for the screen */

/* ST7789V2 Commands */
#define SWRESET 0x01  /* Software Reset */
#define SLP_OUT 0x11  /* Sleep Out */
#define DISP_ON 0x29  /* Display ON */
#define CASET   0x2A  /* Column Address Set */
#define RASET   0x2B  /* Row Address Set */
#define RAMWR   0x2C  /* Write to RAM */
#define MADCTL  0x36  /* Memory Access Control */
#define COLMOD  0x3A  /* Color Mode */

int write_pin(int pin, int state);
int spi_write(uchar *buf, int nbytes);
int spi_read(uchar *buf, int nbytes);
void lcd_reset(void);
void lcd_init(void);
void lcd_cmd(uchar cmd);
void lcd_data(uchar *data, int nbytes);
void lcd_set_addr(uint x, uint y, uint w, uint h);
void lcd_fill(ushort color);
void lcd_square(uint x, uint y, uint size, ushort color);

int fd_gpio;
int fd_spictl;
int fd_spi0;

void main(){
	uchar data[8] = {0};

	fd_gpio = open("/dev/gpio", MAFTER);
	if(fd_gpio < 0){
		bind("#G", "/dev", MAFTER);
		fd_gpio = open("/dev/gpio", MAFTER);
		if(fd_gpio < 0){
			fprint(2, "ERROR: Failed to open GPIO driver file\n");
			return;
		}
	}
	fd_spictl = open("/dev/spictl", ORDWR);
	if(fd_spictl < 0){
		bind("#π", "/dev", MAFTER);
		fd_spictl = open("/dev/spictl", ORDWR);
 		if(fd_spictl < 0){
			fprint(2, "ERROR: Failed to open SPI control file\n");
			close(fd_gpio);
			return;
		}
	}
	fd_spi0 = open("/dev/spi0", ORDWR);
	if(fd_spi0 < 0){
		fprint(2, "ERROR: Failed to open SPI0 data file\n");
		close(fd_gpio);
		close(fd_spictl);
		return;
	}
	fprint(fd_gpio, "function %d out", DC);
	fprint(fd_gpio, "funciton %d out", CS);
	fprint(fd_gpio, "function %d out", RST);
	fprint(fd_spictl, "clock 32000000");
	fprint(fd_spictl, "mode 0\n");

	print("Initializing LCD screen...\n");
	lcd_init();
	print("Setting screen color to black...\n");
	lcd_fill(0x0000);
	print("Drawing a square to the screen...\n");
	lcd_square(50, 50, 100, 0xF800);
	print("Done\n");
	
	return;
}

void lcd_reset(void){
	write_pin(RST, 0);
	sleep(10);
	write_pin(RST, 1);
	sleep(50);
}

void lcd_init(void){
	uchar colmod;
	uchar madctl;

	lcd_reset();
	lcd_cmd(SWRESET);
	sleep(150);
	
	lcd_cmd(SLP_OUT);
	sleep(10);
	
	colmod = 0x55;
	lcd_cmd(COLMOD);
	lcd_data(&colmod, 1);
	sleep(10);
	
	madctl = 0x00;
	lcd_cmd(MADCTL);
	lcd_data(&madctl, 1);

	lcd_cmd(DISP_ON);	
	sleep(10);
}

void lcd_cmd(uchar cmd){
	write_pin(DC, 0);
	spi_write(&cmd, 1);
}

void lcd_data(uchar *data, int nbytes){
	write_pin(DC, 1);
	spi_write(data, nbytes);
}

void lcd_set_addr(uint x, uint y, uint w, uint h){
	uchar buf[4] = {0};
	
	lcd_cmd(CASET);
	buf[0] = (x >> 8);
	buf[1] = (x & 0xFF);
	buf[2] = ((x + w - 1) >> 8);
	buf[3] = ((x + w - 1) & 0xFF);
	lcd_data(buf, 4);

	lcd_cmd(RASET);
	buf[0] = (y >> 8);
	buf[1] = (y & 0xFF);
	buf[2] = ((y + h - 1) >> 8);
	buf[3] = ((y + h - 1) & 0xFF);
	lcd_data(buf, 4);
}

void lcd_fill(ushort color){
	uchar buf[WIDTH * 2] = {0};
	int i;

	lcd_set_addr(0, 0, WIDTH, HEIGHT);
	lcd_cmd(RAMWR);
	
	write_pin(DC, 1);
	for(i = 0; i < WIDTH; i++){
		buf[i * 2] = (color >> 8);
		buf[i * 2 + 1] = (color & 0xFF);
	}

	for(i = 0; i < HEIGHT; i++){
		spi_write(buf, WIDTH * 2);
	}
}

void lcd_square(uint x, uint y, uint size, ushort color){
	uchar buf[WIDTH * 2] = {0};
	int i;
	if(x + size > WIDTH || y + size > HEIGHT){
		fprint(2, "ERROR: Invalid square size\n");
		return;
	}

	lcd_set_addr(x, y, size, size);
	lcd_cmd(RAMWR);
	write_pin(DC, 1);
	for(i = 0; i < size; i++){
		buf[i * 2] = (color >> 8);
		buf[i * 2 + 1] = (color & 0xFF);
	}
	for(i = 0; i < size; i++){
		spi_write(buf, size * 2);
	}
}

int write_pin(int pin, int state){
	int ret;
	if(fd_gpio < 0) return 1;
	ret = fprint(fd_gpio, "set %d %d", pin, (state ? 1 : 0));
	return (ret < 1);
}
int spi_write(uchar *buf, int nbytes){
	if(fd_spi0 < 0) return -1;
	if(nbytes < 1) return 0;
	return write(fd_spi0, buf, nbytes);
}

int spi_read(uchar *buf, int nbytes){
	if(fd_spi0 < 0) return 1;
	if(nbytes < 1) return 0;
	return read(fd_spi0, buf, nbytes);
}
