#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <ctype.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned short int word;
struct display_data
{
	word init_size;
	word width;
	word height;
}display_data;

//#include "/home/pi/Desktop/CEE/TFT/Fonts/raster-fonts-master/IF-12x24.c";
byte iliINITcode[] =
{

					// ili9486 init code from waveshare35a dts file works with linux frame buffer
					0xff,0x01, // soft reset
					0xff,0xb0, 0x00,
					0xff,0x11, // sleep out
					0xfe,0x78,//delay 120 ms
					0xff,0x28,// display off
					0xff,0x3a,0x66,// 18 bit color  0x55 16 bit color only on 3 wire spi BYTE 13 is BITPERPIXEL 18 or 16
					0xff,0x36,0x28, // //0 zero = 0x88 90 = 0xf8 180 = 0x48 270 = 0x28 BYTE 16 is rotate value
					//0xff,0x2A,0x0,0x0,0x01,0xdf,// set columns BYTE 21/22 have to switch with BYTES 29/30 if ROTATE = 0 or 180
					//0xfe,0x10, // 10 ms delay
					//0xff,0x2B,0x0,0x0,0x01,0x3f, // set rows
					//0xfe,0x10, // 10 ms delay
					0xff,0xc2,0x44,
					0xff,0xc5, 0x00, 0x00, 0x00, 0x00,
					// gamma below /
					0xff,0xe0, 0x0f, 0x1f, 0x1c, 0x0c, 0x0f, 0x08, 0x48, 0x98,  0x37, 0x0a, 0x13,  0x04,  0x11, 0x0d, 0x00,
					0xff,0xe1, 0x0f, 0x32, 0x2e, 0x0b, 0x0d, 0x05, 0x47, 0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
					//-1,0xe2, 0x0f, 0x32, 0x2e, 0x0b, 0x0d, 0x05, 0x47, 0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
					// piscreen -> waveshare35a /
					0xff,0x11, // sleep out
					//0xfe,0x0a, // delay 10ms
					0xff,0x29, // display on
					//0xff,0x2C, // write pixel data COMMAND
					0xfd // end of init 85 BYTES

/*
					// startup sequence for MI0283QT-9A
					0xff,0x01, // soft reset
					0xff,0xb0, 0x00,
					0xff,0x11, // sleep out
					0xfe,0x78,//delay 120 ms
					0xff,0x28,// display off
	// ------------memory access control------------------------
					0xff,0x3a,0x66,// 18 bit color  0x55 16 bit color only on 3 wire spi BYTE 13 is BITPERPIXEL 18 or 16
					0xff,0x36,0x28, // //0 zero = 0x88 90 = 0xf8 180 = 0x48 270 = 0x28 BYTE 16 is rotate value
	// ---------------------------------------------------------
					0xff,0xCF, 0x00, 0x83, 0x30,
					0xff,0xED, 0x64, 0x03, 0x12, 0x81,
					0xff,0xE8, 0x85, 0x01, 0x79,
					0xff,0xCB, 0x39, 0X2C, 0x00, 0x34, 0x02,
					0xff,0xF7, 0x20,
					0xff,0xEA, 0x00, 0x00,
	// ------------power control--------------------------------
					0xff,0xC0, 0x26,
					0xff,0xC1, 0x11,
	// ------------VCOM ---------
					0xff,0xC5, 0x35, 0x3E,
					0xff,0xc5, 0x00, 0x00, 0x00, 0x00,
					0xff,0xC7, 0xBE,
		// ------------frame rate-----------------------------------
					//0xff,0xB1, 0x00, 0x1B,
	// ------------Gamma----------------------------------------
	//              0xff,0xF2, 0x08,  //Gamma Function Disable
					//0xff,0x26, 0x01,
	// ------------display--------------------------------------
					0xff,0x2A,0x0,0x0,0x01,0x3f,// set columns BYTE 21/22 have to switch with BYTES 29/30 if ROTATE = 0 or 180
					0xfe,0x10, // 10 ms delay
					0xff,0x2B,0x0,0x0,0x00,0xef, // set rows
					0xfe,0x10, // 10 ms delay
					//0xff,0xB7, 0x07, // entry mode set
					//0xff,0xB6, 0x0A, 0x82, 0x27, 0x00,
					0xff,0x11, // sleep out
					0xfe,0x0a,
					0xff,0x29, // display on
					0xfe,0x0a,
					0xff,0x2c,
					0xfd,
*/
// paste begin:
/*
					// startup sequence for MI0283QT-9A
					0xff,0x01, // software reset
					0xff,0x28, // display off
					0xff,0xb0, 0x0,
					0xff,0x11, // sleep out
					0xfe,0x78,//delay 120 ms

	// ------------memory access control------------------------
					0xff,0x3a,0x66,// 18 bit color  0x55 16 bit color only on 3 wire spi BYTE 13 is BITPERPIXEL 18 or 16
					0xff,0x36,0x28, // //0 zero = 0x88 90 = 0xf8 180 = 0x48 270 = 0x28 BYTE 16 is rotate value
	// ---------------------------------------------------------
					//0xff,0xCF, 0x00, 0x83, 0x30,
					//0xff,0xED, 0x64, 0x03, 0x12, 0x81,
					//0xff,0xE8, 0x85, 0x01, 0x79,
					//0xff,0xCB, 0x39, 0X2C, 0x00, 0x34, 0x02,
					//0xff,0xF7, 0x20,
					//0xff,0xEA, 0x00, 0x00,
	// ------------power control--------------------------------
					0xff,0xC0, 0x26,
					0xff,0xC1, 0x11,
	// ------------VCOM ---------
					0xff,0xC5, 0x00,0x00,0x00,0x00, //0x35, 0x3E,
					0xff,0xc2, 0x44,
					0xff,0xC7, 0xBE,
		// ------------frame rate-----------------------------------
					//0xff,0xB1, 0x00, 0x1B,
	// ------------Gamma----------------------------------------
	                0xff,0xF2, 0x08, //Gamma Function Disable
					//0xff,0x26, 0x01,
	// ------------display--------------------------------------
					0xff,0xB7, 0x07, // entry mode set
					//0xff,0xB6, 0x0A, 0x82, 0x27, 0x00,
					0xff,0x11, // sleep out
					0xfe,0x0a,
					0xff,0x29, // display on
					0xfd,
*/
// paste end

/*
// from adafruit 9341 init code
					0xff,0x01, // software reset
					0xff,0x28, // display off
					0xff,0xb0, 0x0,
					0xff,0x11, // sleep out
					0xfe,0x78,//delay 120 ms

	// ------------memory access control------------------------
					0xff,0x3a,0x66,// 18 bit color  0x55 16 bit color only on 3 wire spi BYTE 13 is BITPERPIXEL 18 or 16
					0xff,0x36,0x28, // //0 zero = 0x88 90 = 0xf8 180 = 0x48 270 = 0x28 BYTE 16 is rotate value

					0xff,0xEF, 0x03, 0x80, 0x02,
					0xff,0xCF, 0x00, 0XC1, 0X30,
					0xff,0xED, 0x64, 0x03, 0X12, 0X81,
					0xff,0xE8, 0x85, 0x00, 0x78,
					0xff,0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,
					0xff,0xF7, 0x20,
					0xff,0xEA, 0x00, 0x00,
					0xff,0xc0, 0x23, // Power control
					0xff,0xc1, 0x10, // Power control
					0xff,0xc5, 0x3e, 0x28, // VCM control
					0xff,0xc7, 0x86, // VCM control2
					0xff,0xb1, 0x00, 0x18,
					0xff,0xb6, 0x08, 0x82, 0x27, // Display Function Control
					0xff,0xF2, 0x00, // Gamma Function Disable
					0xff,0x26, 0x01, // Gamma curve selected
					0xff,0xe0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
								0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00, // Set Gamma
					0xff,0xe1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
								0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F, // Set Gamma
					0xff,0x2A,0x0,0x0,0x01,0x3f,// set columns BYTE 21/22 have to switch with BYTES 29/30 if ROTATE = 0 or 180
					0xfe,0x5, // 5 ms delay
					0xff,0x2B,0x0,0x0,0x00,0xef, // set rows
					0xfe,0x5, // 5 ms delay
					0xff,0xB7, 0x07, // entry mode set
					0x29,
					0xfd,

*/
//***************************************** init code from frame buffer start up  ili9341 ***********************************
/*
0xff,0x01,
0xff,0x28,
0xff,0xcf,
0x00,0x83,0x30,//params
0xff,0xed,
0x64,0x03,0x12,0x81,//params
0xff,0xe8,
0x85,0x01,0x79,//params
0xff,0xcb,
0x39,0x2c,0x00,0x34,0x02,//params
0xff,0xf7,0x20,
0xff,0xea,
0x00,0x00,//params
0xff,0xc0,0x26,
0xff,0x11,
0xff,0xc5,
0x35,0x3e,//params
0xff,0xc7,0xbe,
0xff,0x3a,0x55,
0xff,0x36,0x28,
0xff,0xb1,
0x00,0x1b,//params
0xff,0x26,0x01,
//0xff,0x2A,0x0,0x0,0x01,0x3f,// set columns BYTE 21/22 have to switch with BYTES 29/30 if ROTATE = 0 or 180
//0xfe,0x10, // 10 ms delay
//0xff,0x2B,0x0,0x0,0x00,0xef, // set rows
//0xfe,0x10, // 10 ms delay
0xff,0xb7,0x07,
0xff,0xb6,
0x0a,0x82,0x27,0x00,//params
0xff,0x11,
0xff,0x29,

0xff,0x38,
//0xff,0x2c,
0xfd
*/

//************************** init code ili9488 ***********************
/*
0xff,0x01,
// Interface Mode Control
0xff,0xe9, 0x20,
// Sleep OUT  /
0xff, 0x11,
0xfe,
0x78,
// Pixel Format /
0xff, 0x36, 0x28,//rotated 270 degree's
// 18 bit pixels  /
0xff, 0x3a, 0x66,
// Display mode /
0xff, 0x13,
// Gamma control  /
0xff, 0xc0, 0x08, 0x01,
// CABC control 2 /
0xff, 0xc8, 0xb0,
// DISPON - Display On /
0xff, 0x29,
0xfd,
*/
//**************** working ili9341 init code **************************
/*
0xff,0x01,
0xff,0x28,
0xff,0xcf,
0x00,0x83,0x30,//params
0xff,0xed,
0x64,0x03,0x12,0x81,//params
0xff,0xe8,
0x85,0x01,0x79,//params
0xff,0xcb,
0x39,0x2c,0x00,0x34,0x02,//params
0xff,0xf7,0x20,
0xff,0xea,
0x00,0x00,//params
0xff,0xc0,0x26,
0xff,0x11,
0xfe,0x78, // 120 ms timeout in case we were in sleepin mode
0xff,0xc5,
0x35,0x3e,//params
0xff,0xc7,0xbe,
0xff,0x3a,0x66,// change to 18 bit color 0x55,
0xff,0x36,0x28,// rotated 270 degree's
0xff,0xb1,
0x00,0x1b,//params
0xff,0x26,0x01,
0xff,0xb7,0x07,
0xff,0xb6,
0x0a,0x82,0x27,0x00,//params
0xff,0x11,
0xff,0x38,
0xff,0x29,
0xfd,
*/
};


char *filepath="/media/pi/fa69b178-3404-409f-a3db-7d0d98c660b5/16GigStick/IliTek_Displays/Init_Code/";

char *filename="ili_9486.init";//REMEMBER TO change FILENAME


char pathname[256];
FILE *filePtr;
word bytesWritten;


int main(int argc,char *argv[])
{
word filesize;
char pathway[256];


		filesize=sizeof(iliINITcode);
		display_data.width=480;// REMEMBER to change width and height to reflect display use and orientation in init code
		display_data.height=320;//     "
		display_data.init_size=filesize;
		strcpy(pathway,filepath);
		strcat(pathway,filename);
		filePtr=fopen(pathway,"w");
		if(filePtr==NULL)
		{
			printf("Failed to open %s file! aborting.\n",filename);
			exit(1);
		}
		printf("Data size in bytes %d\n",filesize);
		bytesWritten=fwrite(&display_data,sizeof(byte),sizeof(display_data),filePtr);
		bytesWritten+=fwrite(iliINITcode,sizeof(byte),filesize,filePtr);
		printf("Data written       %d\n",bytesWritten);
		fclose(filePtr);
		if(bytesWritten<filesize+5)
		{
			printf("Failed to write %s file! aborting.\n",filename);
			exit(1);
		}
		printf("File %s written to: %s\n",filename,filepath);
		return(0);
}
