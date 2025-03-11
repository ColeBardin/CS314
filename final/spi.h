#ifndef _SPI_H_
#define _SPI_H_

#include <u.h>
#include <libc.h>

typedef struct
{
    uint fd_spic;
    uint fd_spid;
    uint fd_gpio;
} spi_driver;

int spi_init(spi_driver *);
int cs_init(spi_driver *, uint);
int write_cs(spi_driver *, uint, uchar);
int spi_write(spi_driver *, uchar *, int);
int spi_read(spi_driver *, uchar *, int);

#endif /* _SPI_H_ */
