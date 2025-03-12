#include "spi.h"

int spi_init(spi_driver *spi){
    int fd_gpio;
    int fd_spic;
    int fd_spid;

    if(!spi) return -1;

	fd_gpio = open("/dev/gpio", MAFTER);
	if(fd_gpio < 0){
		bind("#G", "/dev", MAFTER);
		fd_gpio = open("/dev/gpio", MAFTER);
		if(fd_gpio < 0){
			fprint(2, "ERROR: Failed to open GPIO driver file\n");
			return -1;
		}
	}
	fd_spic = open("/dev/spictl", ORDWR);
	if(fd_spic < 0){
		bind("#π", "/dev", MAFTER);
		fd_spic = open("/dev/spictl", ORDWR);
 		if(fd_spic < 0){
			fprint(2, "ERROR: Failed to open SPI control file\n");
			close(fd_gpio);
			return -1;
		}
	}
	fd_spid = open("/dev/spi0", ORDWR);
	if(fd_spid < 0){
		fprint(2, "ERROR: Failed to open SPI0 data file\n");
		close(fd_gpio);
		close(fd_spic);
		return -1;
	}
    spi->fd_gpio = fd_gpio;
    spi->fd_spic = fd_spic;
    spi->fd_spid = fd_spid;

	fprint(fd_spic, "clock 32000000");
	// LTC6813 DS is wrong about what SPI mode to use
	fprint(fd_spic, "mode 2\n");

    return 0;
}

int cs_init(spi_driver *spi, uint cs_pin){
    if(!spi) return -1;

	fprint(spi->fd_gpio, "funciton %d out", cs_pin);
    fprint(spi->fd_gpio, "set %d 1", cs_pin);

    return 0;
}

int write_cs(spi_driver *spi, uint cs_pin, uchar state){
    	if(!spi){
		fprint(2, "write_cs: invalid spi pointer\n");
		return -1;
	}

    	fprint(spi->fd_gpio, "set %d %d", cs_pin, state);

    	return 0;
}

int spi_write(spi_driver *spi, uchar *buf, int nbytes){
    if(!spi) return -1;
	if(nbytes < 1) return 0;

	return write(spi->fd_spid, buf, nbytes);
}

int spi_read(spi_driver *spi, uchar *buf, int nbytes){
    	if(!spi) return -1;
	if(nbytes < 1) return 0;

	return read(spi->fd_spid, buf, nbytes);
}

