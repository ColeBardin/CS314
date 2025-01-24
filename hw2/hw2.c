#include <u.h>
#include <libc.h>

// Using Elegoo DS1307 Module V03
// Contains Maxim/AD DS1307 RTC IC
// 7 bit i2c address: 0x68

void main(){
	int fd_ctl;
	int fd_data;
	uchar data[7] = {0};
	int seconds;
	int minutes;
	int hours;
	int date;
	int month;
	int year;
	
	fd_ctl = open("/dev/i2c.68.ctl", ORDWR);
	if(fd_ctl < 0){
		bind("#J68", "/dev", MAFTER);
		fd_ctl = open("/dev/i2c.68.ctl", ORDWR);
		if(fd_ctl < 0){
			fprint(2, "ERROR: Failed to open I2C control file\n");
			return;
		}
	}
	fd_data = open("/dev/i2c.68.data", ORDWR);
	if(fd_data < 0){
		fprint(2, "ERROR: Failed to open I2C data file\n");
		return;
	}

	if(write(fd_data, data, 7) < 1){
		fprint(2, "ERROR: Failed to write init values to I2C device\n");
		return;
	}
	for(;;){
		if(read(fd_data, data, 7) < 1){
			fprint(2, "ERROR: Failed to read from I2C device\n");
			return;
		}

		// Seconds Register: 0x00
		// [7]    = Clock Halt
		// [6:4] = 10 Seconds
		// [3:0] = Seconds
		seconds = (data[0] & 0xF) + 10 * ((data[0] >> 4) & 0x7);	

		// Minutes Register: 0x01	
		// [7]    = 0
		// [6:4] = 10 Minutes
		// [3:0] = Minutes
		minutes = (data[1] & 0xF) +  10 * ((data[1] >> 4) & 0x7);

		// Hours Regiser: 0x02
		// [7]    = 0
		// [6]    = 0 for 24H, 1 for 12H
		// [5]    = 10 Hours[1] when in 24H mode, AM/PM when in 12H mode
		// [4]    = 10 Hours[0]
		// [3:0] = Hours
		hours = (data[2] & 0xF) + 10 * ((data[2] >> 4) & 0x3);

		// Date Register: 0x04	
		// [7:6] = 0
		// [5:4] = 10 Date
		// [3:0] = Date
		date = (data[4] & 0xF) +  10 * ((data[4] >> 4) & 0x3);

		// Month Register: 0x05
		// [7:5] = 0
		// [4]    = 10 Months
		// [3:0] = Month
		month = (data[5] & 0xF) +  10 * ((data[5] >> 4) & 0x1);

		// Year Register: 0x06
		// [7:4] = 10 Years
		// [3:0] = Year
		year = (data[6] & 0xF) +  10 * ((data[6] >> 4) & 0xF);

		print("%02d/%02d/%04d - %02d:%02d:%02d\n", month, date, year, hours, minutes, seconds);
		sleep(2000);
	}	
}
