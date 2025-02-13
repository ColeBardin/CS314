#include <u.h>
#include <libc.h>

#define CS 21 /* Pin 21 is chip select for SPI client */

int set_cs(int fdspi, int state);

void main(){
	int fd_gpio;
	int fd_spi0;
	int fd_spictl;

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

	return;
}

int set_cs(int fdspi, int state){
	int ret;
	if(fdspi < 0) return 1;
	ret = fprint(fdspi, "set %d %d", CS, state ? 1 : 0);
	return (ret < 1);
}