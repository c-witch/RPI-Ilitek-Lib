/*
 * Ilitek library for the 9340,9341,9486,9488 LCD Controller
 *
 * Copyright (C) 2021 Brina Keith
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Honoralble mention to the following for supplying the frame buffer
 * driver code from which the init code was gleamed!
 *
 *
 * Christian Vogelgsang ili9341
 * Noralf Tronnes       ili9340,ili9486
 * Sachin Surendran     ili9488
 */
#include "/home/pi/IliTek_Displays/Includes/extrafunctions.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <gpiod.h>
#include <spidev.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/spi/spidev.h>

#define DEBUG_INIT 0// =1 will display init code being sent to display during initialization


#if!defined word
typedef unsigned short int word;
#endif
#if!defined byte
typedef unsigned char byte;
#endif
#if!defined LOW
#define LOW 0
#endif
#if!defined HIGH
#define HIGH 1
#endif
#if!defined True
#define True 1
#endif
#if!defined False
#define False 0
#endif
#if!defined ON
#define ON 1
#endif
#if!defined OFF
#define OFF 0
#endif


#define HORIZONTAL 0
#define VERTICAL 1
#define CH_ONLY 2 // character bits only to textcolor
#define CH_BG 4 // character bits to textcolor and other bits set to textbkcolor
#define CH_INV 8 // swap text and background color for characters
#define CH_UL 16 // underline characters with textcolor


#define Fill 1
#define Nofill 0
//#define Degree 248 // character code for degree symbol
#define OpenArc 0
#define ClosedArc 1



typedef word cords[2];
cords triangle[3];

word INITBYTES;

char *errMsg[20] =
{
"OK\n",//0
"Invalid direction\n",//1
"Character off edge of screen\n",//2
"Character off bottom of screen\n",//3
"String is to long for the screen position and direction\n",//4
"Can't have vertical AND underline\n",//5
"X or X1 is out of range\n",//6
"Y or Y1 is out of range\n",//7
"Unable to allocate memory\n",//8
"Invalid rotation angle\n",//9
"Invalid start / end angles\n",//10
"Arc radius to large\n",//11
"Invalid mode\n",//12
"Circle radius to large\n",//13
"Ellipse X radius to large\n",//14
"Ellipse Y radius to large\n",//15
"Invalid Font Index\n",//16
"Invalid Path and/or Font name\n",//17


};


byte *gptr = NULL;
byte *Rxptr= NULL;
byte *GPTR=NULL;
byte *RXPTR=NULL;
byte Foreground[3],Background[3],Textcolor[3],TextBkcolor[3];
word lcdWidth = 320;//480;
word lcdHeight = 240;//480;//240;//320;
word scanlinebytes;
word rotateAngle = 270;
byte bgr = 1;
unsigned int writeBytes,bytesperscreen;;
//clock_t tick1,tick2, startFill, endFill, time2Fill;

//****************** Font data below ***********************************
/*
#define f09x16 0
#define f12x16 1
#define f10x20 2
#define f12x23 3
#define f12x24 4
#define f12x27 5
#define f14x30 6
#define f16x32 7
*/
char *FontNames[8]=
{
"f09x16.fbm",
"f12x16.fbm",
"f10x20.fbm",
"f12x23.fbm",
"f12x24.fbm",
"f12x27.fbm",
"f14x30.fbm",
"f16x32.fbm",
};

char *fPath = "/home/pi/IliTek_Displays/Fonts/";
word font[16388];
word fontWidth,fontHeight;
word *txtptr=NULL;// pointer into character glyphs for char to print
byte FontControlFlag = 2;
byte GraphicsControlFlag = 0;
#define BITBLIT 1

//********************** Display data below ****************************
#define MEM_Y   (7) // MY row address order
#define MEM_X   (6) // MX column address order
#define MEM_V   (5) // MV row / column exchange
#define MEM_L   (4) // ML vertical refresh order
#define MEM_H   (2) // MH horizontal refresh order
#define MEM_BGR (3) // RGB-BGR Order

byte iliINITcode[512];

#define iliLCDs 4
char *iliChips[iliLCDs] =
{
"ILI9341",// 7 Bytes
"ILI9486",
"ILI9488",
""
};
/*
#define ILIparams 20
char *iliParams[ILIparams] =
{
"Void",
"Display",
"DC",
"RESET",
"LED",
"BGR",
"ROTATE",
"SPI",
"?",
"H",
};
*/
#define ILIinit 4

char *iliINIT[ILIinit] =
{
"ili_9340.init",
"ili_9341.init",
"ili_9486.init",
"ili_9488.init",
};

FILE *initFD = NULL;
word initSize=0;// size of initialization file in bytes
word ROTATE = 270;
byte displayRotate=0x28;
word initParamCount=0;
byte initList[20];
word iliINITindex = 2;
byte lcdBGR = 1;
word DC = 24;    // default DC GPIO number
word RESET = 23; // default RESET GPIO number
word LED = 18;   // default BACKLIGHT GPIO number
word lcdError=-1;
//byte *initCodePtr = NULL;
//************************ SPI data below ******************************

static int SPI_index = 0;
static char SPI_NAME[15];
static int SPIspeed = 32; // MHz

static char *SpiNames[5] =
{
"SPI0_0",
"SPI0_1",
"SPI1_0",
"SPI1_1",
"SPI1_2",
};

static const char       *spiDevice[5] =
{
"/dev/spidev0.0",
"/dev/spidev0.1",
"/dev/spidev1.0",
"/dev/spidev1.1",
"/dev/spidev1.2",
};
static const uint8_t     spiBPW   = 8 ;
static const uint16_t    spiDelay = 0 ;
int lcdFD=0;
static uint32_t    spiSpeeds [5] ;
static int         spiFds [5] ;

struct gpiod_chip *pins_chip;
struct gpiod_line *Led,*Reset,*Dc;
/*
#define BITmapIMAGES 8
char *Bmp_Demo[BITmapIMAGES]=
{
"Maple_Leaf.bmp",
"Tux_Linux.bmp",
"Petpeswick_Inlet2_Ili.bmp",
"PrivateerAtHome_IliImage.bmp",
"SunSetOnPetpeswickInlet_ili.bmp",
"Hubble-Telescope-Takes-Close-up-Portrait-of-Jupiter.bmp",
"spiral_galaxy.bmp",
"bunny.bmp",
};
*/
//************************Proto Types***********************************

// *********** Utility functions ***************************************
void color18(byte *colorptr,int color);
void EXIT(byte err);
word lcdChk_Key(void);
void hardReset(void);
void bitprint(word lint,word size);
word lcdOrientation(word rotation);
word stringPixels(char* str);
byte lcdOpenFont(byte index);
void lcdErrorMesg(byte index);
byte lcdShowBmp(char *fileName);
//************ SPI related I/O Functions *******************************
int writeCommand(byte);
int writeData(byte*,word);
//*********** Utility display functions ********************************
void lcdon(void);
void lcdoff(void);
void lcdInvert(word);
//*********** Host memory to display functions *************************
unsigned int lcdOffset(word ,word);
int bitBlt(word,word,word,word);
int lcdRefresh(void);
unsigned int lcdClear(void);
int lcdSetwindow(int,int,int,int);
//*********** Graphic primatave functions ******************************
int lcdPutpixel(word x,word y);
unsigned int lcdGetpixel(word ,word );
int lcddrawChar(word,word ,char*);
int lcddrawString(int,int,char *);
int lcddrawHline(word,word,word);
int lcddrawVline(word,word,word);
byte lcddrawLine(int ,int ,int ,int);
int lcddrawRectangle(word,word,word,word,byte);
word lcdFillBackground(void);
word lcdclearArea(word,word,word,word);
byte lcddrawArc(word ,word ,word ,word ,word ,byte);
byte lcddrawCircle(word ,word ,word ,byte);
byte lcddrawEllipse(word ,word ,word ,word ,byte);
int lcdrestoreArea(word ,word ,byte *);
byte* lcdsaveArea(word ,word ,word ,word );
int lcddrawTriangle(word x0,word y0,word radius,byte type,word ang1,word ang2,word ang3);
//******************* GPIO pins ****************************************
void gpioOpen(void);
void gpioPins(void);
void gpioClose(void);
void DigitalWrite(struct gpiod_line *,word);
//******************* SPI **********************************************
int SPIGetFd (int channel);
int SPIDataRW (int channel, unsigned char *Txptr,int len);
int SPISetupMode (int channel, int speed, int mode);
int SPISetup (char *, int speed);
//************************ Entry and Exit functions ********************
void lcdOpenDisplay(byte fontdex);
void lcdCloseDisplay(void);
//*********************** End proto types ******************************


//********************* SPI functions **********************************

int SPIGetFd (int channel)
{
	return spiFds [channel & 7] ;
}
// *********** workhorse function below does all heavy work ************
int SPIDataRW (int SPI_index, unsigned char *Txptr,int len)
{
struct spi_ioc_transfer spi ;
short int err;

	SPI_index &= 7 ;
	if(Rxptr!=NULL)
		memset(Rxptr,0,4096);
	memset (&spi, 0, sizeof (spi)) ;
	spi.tx_buf        = (unsigned long)Txptr ;
	spi.rx_buf        = (unsigned long)Rxptr ;
	spi.len           = len ;
	spi.delay_usecs   = spiDelay ;
	spi.speed_hz      = spiSpeeds [SPI_index] ;
	spi.bits_per_word = spiBPW ;
	err=ioctl (spiFds [SPI_index], SPI_IOC_MESSAGE(1), &spi) ;
	if(err<0)
	{
		printf("%d SPI write error %s\n",err,strerror(errno));
		EXIT(1);
	}
	return(0);
}

int SPISetupMode (int index, int speed, int mode)
{
int fd ;


	mode    &= 3 ;	// Mode is 0, 1, 2 or 3
	index &= 0b00000111 ;	// Channel is 0 to 4 ie spi0.0 spi0.1 spi1.0 spi1.1 spi1.2
	if((fd=open(spiDevice[SPI_index],O_RDWR | O_NONBLOCK))<0)
	{
    fprintf(stderr,"%s  %d\n",spiDevice[SPI_index],SPI_index);
    fflush(stderr);
    printf("Unable to open SPI device: %s\n", strerror (errno));
    EXIT(1);
	}
	spiSpeeds [SPI_index] = speed ;
	spiFds    [SPI_index] = fd ;
	if (ioctl (fd, SPI_IOC_WR_MODE, &mode)< 0)
	{
		printf("SPI Mode Change failure: %s\n", strerror (errno));
		EXIT(1);
	}
	if (ioctl (fd, SPI_IOC_WR_BITS_PER_WORD, &spiBPW)< 0)
	{
		printf("SPI BPW Change failure: %s\n", strerror (errno)) ;
		EXIT(1);
	}
	if (ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)< 0)
	{
		printf("SPI Speed Change failure: %s\n", strerror (errno)) ;
		EXIT(1);
	}
	return fd ;
}

int SPISetup (char *SpiNamePtr,int speed )
{
int x;

	switch(speed)
	{
		case 1 :
		case 2 :
		case 4 :
		case 8 :
		case 16 :
		case 32 :
			speed *= 1000000;
			break;
		default :
			speed = 1000000;
			SPIspeed=1;
			break;
	}
	for(x=0;x<5;++x)
	{
		if((strcasecmp(SpiNames[x],SpiNamePtr)==0))
		{
			SPI_index=x;
			return(SPISetupMode(SPI_index,speed,0));
		}
	}
	SPI_index=0;
	return SPISetupMode (SPI_index, speed, 0) ;
}
//***************** End of SPI functions *******************************


// ************************** GPIO functions ***************************

void gpioOpen(void)
{
	pins_chip = gpiod_chip_open("/dev/gpiochip0");
	if (pins_chip==NULL)
	{
		printf("Fatal: Unable to open GPIO! Aborting\n");
		printf("%s\n",strerror(errno));
		EXIT(1);
	}
}

void gpioClose(void)
{
	gpiod_line_set_value(Led,LOW);
	gpiod_line_release(Led);
	gpiod_line_release(Reset);
	gpiod_line_release(Dc);
	gpiod_chip_close(pins_chip);
}

void gpioPins(void)
{
word rc;

	Led = gpiod_chip_get_line(pins_chip, LED);
	if(Led==NULL)
	{
		printf("LED Pin unavailable\n");
		gpiod_chip_close(pins_chip);
		EXIT(1);
	}
	Reset = gpiod_chip_get_line(pins_chip, RESET);
	if(Reset==NULL)
	{
		printf("RESET Pin unavailable\n");
		gpiod_chip_close(pins_chip);
		EXIT(1);
	}
	Dc = gpiod_chip_get_line(pins_chip, DC);
	if(Dc==NULL)
	{
		printf("DC Pin unavailable\n");
		gpiod_chip_close(pins_chip);
		EXIT(3);
	}
	rc=gpiod_line_request_output(Led,"led", LOW);
	if (rc<0)
	{
		gpiod_chip_close(pins_chip);
		printf("Fatal! Led Pin unavailable. Aborting\n");
		EXIT(1);
	}
	rc=gpiod_line_request_output(Reset,"reset", HIGH);
	if (rc<0)
	{
		gpiod_chip_close(pins_chip);
		printf("Fatal! Reset Pin unavailable. Aborting\n");
		EXIT(1);
	}
	rc=gpiod_line_request_output(Dc,"d/c", HIGH);
	if (rc<0)
	{
		gpiod_chip_close(pins_chip);
		printf("Fatal! D/C Pin unavailable. Aborting\n");
		EXIT(1);
	}
	return;
}

void DigitalWrite(struct gpiod_line *pin ,word value)
{
word err;

	err=gpiod_line_set_value(pin,value);
	if(err<0)
	{
		printf("Fatal error writting GPIO: %s\n",strerror(errno));
		EXIT(1);
	}
	return;
}

//********************** End GPIO functions ****************************
//
//****************** Program general functions *************************
//

word lcdChk_Key(void)
{
word key;

	if(kbhit())
		key=getchar();
	else
		return(0);
	switch(key)
	{
		case 'b' :
			if(GraphicsControlFlag & 0b01000000)
			{
				GraphicsControlFlag &= 0b10111111;
				lcdoff();
			}
			else
			{
				GraphicsControlFlag |= 0b01000000;
				lcdon();
			}
			key=0;
			break;
		case 'q' :
			EXIT(0);
			break;
		default :
			break;
	}
	return(key);
}

byte lcdShowBmp(char *fileName)
{
byte dummyread,BmpHeader[54];
byte *pixptr,*baseptr,*holdptr;
int x,y,startx,starty,bytes2read;
char str[3];
int bmpFD, bytesRead;	/// Sas changed this from WORD to INT
char filePath[255];
int *offset,*filesize,*width,*height,Width,Height,size,size1;
short int *pixelformat;
byte savcolour[3];

	strcpy(filePath,"/home/pi/IliTek_Displays/Image/");
	strcat(filePath,fileName);
	bmpFD = open (filePath, O_RDONLY);
	if (bmpFD < 0)
	{
		lcdError = 18;
		printf("Could not open %s / %s\n",fileName,strerror(errno));								///			Hi Bunny! :) It Works Now!			///
		return (1);									///		Saved backup as V4B2. This is V4B3.		///
	}

	bytesRead = read (bmpFD, &BmpHeader, 54);		/// <-- Added missing ampersand here: &BmpHeader

	if (bytesRead <54)
	{
		close (bmpFD);
		printf("Failed to read image header\n");
		lcdError = 19;
		return (1);
	}
	memset(str,0,sizeof(str));
	str[0]=BmpHeader[0];
	str[1]=BmpHeader[1];
	filesize=(int*)&BmpHeader[2];
	offset=(int*)&BmpHeader[10];
	width=(int*)&BmpHeader[18];
	height=(int*)&BmpHeader[22];
	pixelformat=(short int*)&BmpHeader[28];
	Width=*width;
	Height=*height;
	if(!(strcasecmp(str,"BM")==0))
	{
		lcdError=20;
		printf("File Not a BMP\n");
		return(1);
	}
	if(Width>lcdWidth)
	{
		lcdError=21;
		printf("Image too wide\n");
		return(1);
	}
	if(Height>lcdHeight)
	{
		lcdError=22;
		printf("Image too tall\n");
		return(1);
	}
	if(*pixelformat != 0x18)
	{
		lcdError=23;
		printf("Image must be 24bpp\n");
		return(1);
	}
	for(x=0;x<3;++x)
		savcolour[x]=Background[x];
	size=*filesize-*offset;
	size1=Width*Height*3;
	size1=size-size1;
	starty=(lcdHeight-Height)/2;
	startx=(lcdWidth-Width)/2;
	bytes2read=Width*3;
	pixptr=gptr;
	dummyread=Width%4;//2
	pixptr=malloc(size);
	baseptr=pixptr;
	holdptr=pixptr;
	pixptr+=(size-bytes2read-(size1));//size1
	for(y=0/*starty*/;y<Height;++y)
	{
		read(bmpFD,pixptr,bytes2read);
		if(dummyread)
			read(bmpFD,str,dummyread);//(size1/Height));//1
		pixptr-=bytes2read;
	}
	if((Width<lcdWidth) || (Height<lcdHeight))
	{
		Background[2]=*(baseptr+0)&253;
		Background[1]=*(baseptr+1)&253;
		Background[0]=*(baseptr+2)&253;
		lcdFillBackground();
	}
	else
	{
		for(x=0;x<3;++x)
			savcolour[x]=Background[x];
	}
	for(y=starty;y<Height+starty;++y)
	{
		pixptr=gptr+lcdOffset(startx,y);
		for(x=0;x<Width;++x)
		{
			*(pixptr+0)=*(baseptr+2)&253;
			*(pixptr+1)=*(baseptr+1)&253;
			*(pixptr+2)=*(baseptr+0)&253;
			pixptr+=3;
			baseptr+=3;
		}
	}
	for(x=0;x<3;++x)
		Background[x]=savcolour[x];
	pixptr=holdptr;
	free(pixptr);
	lcdRefresh();
	close(bmpFD);
	return (0);
}

word lcdOrientation(word rotation)
{
byte rotate;
word temp;

	switch(rotation)
	{
		case 0 :
			rotate=(1 << MEM_X) | (lcdBGR << MEM_BGR);
			if((ROTATE==90) || (ROTATE==270))
			{
				temp=lcdWidth;
				lcdWidth=lcdHeight;
				lcdHeight=temp;
				scanlinebytes=lcdWidth*3;
				ROTATE=0;
			}
			break;
		case 90 :
			rotate=(1 << MEM_Y) | (1 << MEM_X) |
			(1 << MEM_V) | (lcdBGR << MEM_BGR);
			if((ROTATE==0) || (ROTATE==180))
			{
				temp=lcdWidth;
				lcdWidth=lcdHeight;
				lcdHeight=temp;
				scanlinebytes=lcdWidth*3;
				ROTATE=90;
			}
			break;
		case 180 :
			rotate=(1 << MEM_Y) | (lcdBGR << MEM_BGR);
			if((ROTATE==90) || (ROTATE==270))
			{
				temp=lcdWidth;
				lcdWidth=lcdHeight;
				lcdHeight=temp;
				scanlinebytes=lcdWidth*3;
				ROTATE=180;
			}
			break;
		case 270 :
			rotate=(1<<MEM_V) | (1 << MEM_L) | (lcdBGR << MEM_BGR);
			if((ROTATE==0) || (ROTATE==180))
			{
				temp=lcdWidth;
				lcdWidth=lcdHeight;
				lcdHeight=temp;
				scanlinebytes=lcdWidth*3;
				ROTATE=270;
			}
			break;
		default :
			lcdError=9;
			return(1);
	}
	writeCommand(0x0);// NOP
	writeCommand(0x28);//display off
	writeCommand(0x36);//command to change rotation
	writeData(&rotate,1);// rotation data
	writeCommand(0x29);//turn display on
	writeCommand(0x2c);//put display into graphic data input mode
	if(GraphicsControlFlag & 0x80)
		GraphicsControlFlag ^= 0x80;
	return(0);
}



void hardReset(void) // Hard reset of display
{
	DigitalWrite(Reset,HIGH);
	usleep(5*1000);
	DigitalWrite(Reset,LOW);
	usleep(20*1000);
	DigitalWrite(Reset,HIGH);
	usleep((150*1000));
	return;
}

void EXIT(byte err)
{
	if(gptr!=NULL)
		free(gptr);
	if(Rxptr!=NULL)
		free(Rxptr);
	if(KBinitialized)
		kbfini();
	if(pins_chip != NULL)
	{
		lcdoff();
		gpioClose();
		hardReset();
	}
	if(lcdFD>0)
	{
		close(lcdFD);
	}
	exit(err);
}

void lcdon(void)
{
	DigitalWrite(Led,HIGH);
	return;
}

void lcdoff(void)
{
	DigitalWrite(Led,LOW);
	return;
}

void lcdInvert(word mode)
{
	switch(mode)
	{
		case ON :
			writeCommand(0x21);
			break;
		case OFF :
			writeCommand(0x20);
			break;
		default :
			break;
	}
}

int sgn(int value) // used by drawline function
{
int v;
	if(value<0)
		v=-1;
	if(value==0)
		v=0;
	if(value>0)
		v=1;
	return v;
}

void color18(unsigned char * colorptr,int color)//conv 24bit color to 18 bit
{
	*colorptr=(color>>16)&252;
	*(colorptr+1)=(color>>8)&252;
	*(colorptr+2)=color&252;
}

void lcdErrorMesg(byte index)
{
	printf("%s\n",errMsg[index]);
	return;
}

int lcdSetwindow(int x,int y,int x1,int y1)// set pixel location x,y and window pixel width / pixel height
{
static byte buffer[4];
word temp;

	writeCommand(0x0);
	writeCommand(0x0);
	memset(buffer,0,sizeof(buffer));
	if(x>x1)
	{
		temp=x;
		x=x1;
		x1=temp;
	}
	if(x<0)
		x=0;
	if(x1>lcdWidth-1)
		x1=lcdWidth-1;
	buffer[0]=x>>8;
	buffer[1]=x&0xff;
	buffer[2]=x1>>8;
	buffer[3]=x1&0xff;
	writeCommand(0x2A);
	writeData(&buffer[0],1);
	writeData(&buffer[1],1);
	writeData(&buffer[2],1);
	writeData(&buffer[3],1);
	memset(buffer,0,sizeof(buffer));
	if(y>y1)
	{
		temp=y;
		y=y1;
		y1=temp;
	}
	if(y<0)
		y=0;
	if(y1>lcdHeight-1)
		y1=lcdHeight-1;
	buffer[0]=y>>8;
	buffer[1]=y&0xff;
	buffer[2]=y1>>8;
	buffer[3]=y1&0xff;
	writeCommand(0x2B);
	writeData(&buffer[0],1);
	writeData(&buffer[1],1);
	writeData(&buffer[2],1);
	writeData(&buffer[3],1);
	writeCommand(0x2C);// begin pixel output @ x,y and wrap at x1 y1
	return(0);
}


int writeCommand(byte command) // write command to data spi
{
static byte c;

	c=command;
	DigitalWrite(Dc,LOW);
	usleep(1);
	SPIDataRW (SPI_index,&c,1);
	DigitalWrite(Dc,HIGH);
	return(0);
}

int writeData(byte *txptr,word count) // write data to spi
{
	SPIDataRW (SPI_index,txptr,count);
	return(0);
}

int lcdinitPanel(void)// initialize the display
{
int count;

	count=0;
	while(count<INITBYTES && (iliINITcode[count]!=0xfd))
	{
		switch(iliINITcode[count])
		{
			case 0xff :
				if(DEBUG_INIT)
					printf("\n");
				++count;
				if(DEBUG_INIT)
					printf("%02x ",iliINITcode[count]);
				writeCommand(iliINITcode[count]);
				++count;
				break;
			case 0xfe :
				++count;
				if(DEBUG_INIT)
					printf("\nTime out for %d mS",iliINITcode[count]);
				usleep(iliINITcode[count]*1000);
				++count;
				break;
			default :
				if(DEBUG_INIT)
					printf("%02x ",iliINITcode[count]);
				writeData(&iliINITcode[count],1);
				++count;
				break;
		}
	}
	if(DEBUG_INIT)
		printf("\n");
	return(count);
}


unsigned int lcdOffset(word x,word y)//get offset into host vid memory
{
unsigned int oset;

	if((x<0) || (x>lcdWidth-1))
	{
		lcdError=6;
		return(1);
	}
	if((y<0) || (y>lcdHeight-1))
	{
		lcdError=7;
		return(1);
	}
	oset=((y*scanlinebytes)+(x*3));
	return(oset);
}

int lcdRefresh(void)//does complete host memory to display transfer
{
unsigned int index,blocks,pblocks;
static byte* pixptr;

	lcdSetwindow(0,0,lcdWidth-1,lcdHeight-1);
	pixptr=gptr;
	blocks=bytesperscreen/4096;
	pblocks=bytesperscreen%4096;
	for(index=0;index<blocks;++index)
	{
		writeData(pixptr,4096);
		pixptr+=4096;
	}
	if(pblocks>0)
		writeData(pixptr,pblocks);
	return(0);
}

unsigned int lcdClear(void)// clears display to Background colour
{
unsigned int count,index;
static byte* pixptr;

	pixptr=gptr;
	memset(pixptr,0,bytesperscreen);
	count=lcdWidth*lcdHeight;//*lcdHeight*3;
	for(index=0;index<count;++index)
	{
		*pixptr=Background[0];
		*(pixptr+1)=Background[1];
		*(pixptr+2)=Background[2];
		pixptr+=3;
	}
	bitBlt(0,0,lcdWidth-1,lcdHeight-1);
	return(0);
}


int bitBlt(word x1,word y1,word x2,word y2)// blasts a region or full screen to display from host memory
{
static byte *pixptr,*basptr,*hostptr;
unsigned int x,y,blocks,pblocks;

	if(x1>x2)
	{
		x=x1;
		x1=x2;
		x2=x;
	}
	if(y1>y2)
	{
		y=y1;
		y1=y2;
		y2=y;
	}
	x=x2-x1+1;
	y=y2-y1+1;
	pixptr=malloc(x*y*3);
	if(pixptr==NULL)
	{
		printf("Fatal: Unable to allocate Blit buffer aborting program\n");
		EXIT(1);
	}
	basptr=pixptr;
	writeBytes=(x2-x1+1)*(y2-y1+1)*3;
	blocks=(word)(writeBytes/4096);
	pblocks=writeBytes%4096;
	for(y=y1;y<=y2;++y)
	{
		hostptr=gptr+lcdOffset(x1,y);
		for(x=x1;x<=x2;++x)
		{
			*pixptr=*(hostptr);
			*(pixptr+1)=*(hostptr+1);
			*(pixptr+2)=*(hostptr+2);
			hostptr+=3;
			pixptr+=3;
		}
	}
	pixptr=basptr;
	lcdSetwindow(x1,y1,x2,y2);
	for(y=0;y<blocks;++y)
	{
		writeData(pixptr,4096);
		pixptr+=4096;
	}
	if(pblocks>0)
		writeData(pixptr,pblocks);
	free(basptr);
	return(0);
}

int lcdrestoreArea(word x,word y,byte *resptr)//restores area of host memory gotten with save area !! doesn't release point after doing so
{
word *intptr,width,height,lin,col;
byte*pixptr;
unsigned int oset;

	intptr=(word*)resptr;
	width=*intptr;
	height=*(intptr+1);
	if(x<0)
	{
		lcdError=6;
		return(-1);
	}
	if(y<0)
	{
		lcdError=7;
		return(-1);
	}
	if(x+width>lcdWidth-1)
	{
		lcdError=6;
		return(-1);
	}
	if(y+height>lcdHeight-1)
	{
		lcdError=7;
		return(-1);
	}
	resptr+=4;
	oset=lcdOffset(x,y);
	for(lin=0;lin<height;++lin)
	{
		pixptr=gptr+oset+(lin*scanlinebytes);
		for(col=0;col<width;++col)
		{
			*(pixptr+(col*3))=*(resptr+(col*3));
			*(pixptr+(col*3)+1)=*(resptr+(col*3)+1);
			*(pixptr+(col*3)+2)=*(resptr+(col*3)+2);
		}
	}
	bitBlt(x,y,x+width-1,y+height-1);
	return(0);
}

byte* lcdsaveArea(word x,word y,word x1,word y1)//save area of host memory to a save buffer
{
byte *pixptr,*savptr,*basptr;
word temp,width,height,lin,col,*intptr;
int oset,bytecount;

	if(x>x1)
	{
		temp=x;
		x=x1;
		x1=temp;
	}
	if(y>y1)
	{
		temp=y;
		y=y1;
		y1=temp;
	}
	if((x<0) || (x1>lcdWidth-1))
	{
		lcdError=6;
		return (NULL);
	}
	if((y<0) || (y1>lcdWidth-1))
	{
		lcdError=7;
		return(NULL);
	}
	width=x1-x+1;
	height=y1-y+1;
	bytecount=width*height*3;
	savptr=malloc(bytecount+4);
	basptr=savptr;
	if(savptr==NULL)
	{
		lcdError=8;
		return(NULL);
	}
	intptr=(word*)savptr;
	*intptr=width;
	*(intptr+1)=height;
	savptr+=4;
	oset=lcdOffset(x,y);
	for(lin=0;lin<height;++lin)
	{
		pixptr=gptr+oset+(lin*scanlinebytes);
		for(col=0;col<width;++col)
		{
			*savptr=*(pixptr+(col*3)+0);
			*(savptr+1)=*(pixptr+(col*3)+1);
			*(savptr+2)=*(pixptr+(col*3)+2);
			savptr+=3;
		}
	}
	lcdError=0;
	return(basptr);
}


int lcdPutpixel(word x,word y)// writes single pixel to host memory and dislay
{
byte *pixptr;

	if((x<0) || (x>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y<0) || (y>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	pixptr=gptr+lcdOffset(x,y);
	*pixptr=Foreground[0];
	*(pixptr+1)=Foreground[1];
	*(pixptr+2)=Foreground[2];
	pixptr=Foreground;
	if(GraphicsControlFlag & BITBLIT)
		return(0);
	lcdSetwindow(x,y,x,y);
	writeData(pixptr,3);
	return(0);
}

unsigned int lcdGetpixel(word x,word y) //returns pixel colour value as unsigned int
{
byte *pixptr,*retptr;
static unsigned int retColor;

	if((x<0) || (x>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y<0) || (y>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	retptr=(byte*)&retColor;
	pixptr=gptr;
	pixptr+=lcdOffset(x,y);
	*(retptr +2 )=*pixptr;
	*(retptr +1) = *(pixptr+1);
	*(retptr) = *(pixptr+2);
	return(retColor);
}

int lcddrawHline(word x1,word x2,word y1)//draws horizontal lines
{
byte static *pixptr; //this contains pointer to host memory being written
unsigned int oset;// this contains offset from host memory to starting x position from host memory
int z; // contains error code if any otherwise 0
word temp;


	if((x1<0) || (x1>lcdWidth-1))
	{
		lcdError=6;
		return(1);
	}
	if((y1<0) || (y1>lcdHeight-1))
	{
		lcdError=7;
		return(1);
	}
	if((x2<0) || (x2>lcdWidth-1))
	{
		lcdError=6;
		return(1);
	}
	if(x1>x2)
	{
		temp=x1;
		x1=x2;
		x2=temp;
	}
	oset=lcdOffset(x1,y1);
	pixptr=gptr+oset;
	for(z=x1;z<=x2;z++)
	{
		*(pixptr)=Foreground[0];
		*(pixptr+1)=Foreground[1];
		*(pixptr+2)=Foreground[2];
		pixptr+=3;
	}
	if(GraphicsControlFlag&1)
		return(0);
	else
		bitBlt(x1,y1,x2,y1);
	return(0);
}

int lcddrawVline(word x1,word y1,word y2)//draws vertical lines
{
static byte *pixptr; //this contains pointer to host memory being written
unsigned int oset;// this contains offset from host memory to starting x position from host memory
int z;
word temp;

	if((x1<0) || (x1>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y1<0) || (y1>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if((y2<0) || (y2>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if(y1>y2)
	{
		temp=y1;
		y1=y2;
		y2=temp;
	}
	oset=lcdOffset(x1,y1);
	pixptr=gptr+oset;
	for(z=y1;z<=y2;z++)
	{
		*pixptr=Foreground[0];
		*(pixptr+1)=Foreground[1];
		*(pixptr+2)=Foreground[2];
		pixptr+=scanlinebytes;
	}
	if(GraphicsControlFlag&1)
		return(0);
	else
		bitBlt(x1,y1,x1,y2);
	return(0);
}

int lcddrawRectangle(word x1,word y1,word x2,word y2,byte mode)//draws rectangles filled or not
{
word temp, y;

	if((x1<0) || (x1>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((x2<0) || (x2>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y1<0) || (y1>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if((y2<0) || (y2>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if(x1>x2)
	{
		temp=x1;
		x1=x2;
		x2=temp;
	}
	if(y1>y2)
	{
		temp=y1;
		y1=y2;
		y2=temp;
	}
	GraphicsControlFlag |= BITBLIT;
	switch(mode)
	{
		case Nofill:
			lcddrawHline(x1,x2,y1);
			lcddrawHline(x1,x2,y2);
			lcddrawVline(x1,y1,y2);
			lcddrawVline(x2,y1,y2);
			break;
		case Fill :
			for(y=y1;y<=y2;++y)
				lcddrawHline(x1,x2,y);
			break;
		default :
			return(-7);
	}
	GraphicsControlFlag ^= BITBLIT;
	bitBlt(x1,y1,x2,y2);
	return(0);
}

word lcdFillBackground(void)
{
byte *pixptr;
word x,y;

	for(y=0;y<lcdHeight;++y)
	{
		pixptr=gptr+(y*scanlinebytes);
		for(x=0;x<lcdWidth;++x)
		{
			*(pixptr)=Background[0];
			*(pixptr+1)=Background[1];
			*(pixptr+2)=Background[2];
			pixptr+=3;
		}
	}
	return(0);
}

word lcdclearArea(word x1,word y1,word x2,word y2)//clears area of host memory and display with Foreground colour
{
byte saveColor[3];
word temp,z;

	if((x1<0) || (x1>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((x2<0) || (x2>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y1<0) || (y1>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if((y2<0) || (y2>lcdHeight-2))
	{
		lcdError=7;
		return(-1);
	}
	if(x1>x2)
	{
		temp=x1;
		x1=x2;
		x2=temp;
	}
	if(y1>y2)
	{
		temp=y1;
		y1=y2;
		y2=temp;
	}
	for(z=0;z<3;++z)
	{
		saveColor[z]=Foreground[z];
		Foreground[z]=Background[z];
	}
	lcddrawRectangle(x1,y1,x2,y2,Fill);
	for(z=0;z<3;++z)
		Foreground[z]=saveColor[z];
	return(0);
}

byte lcddrawArc(word x0,word y0,word start,word end,word radius,byte mode)// draws open or closed arc no fill at this time
{
int x,y,x1,y1;
double radian,s_start,s_end,dstep,degree;
byte* pixptr;
word x2,y2;

	if((mode<0) || (mode>1))
	{
		lcdError=12;
		return(-1);
	}
	if(end==start)
	{
		lcdError=10;
		return(-1);
	}
	if((x0<0) || (x0>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y0<0) || (y0>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if(end<start)
		end+=360;
	if((x0-radius<0) || (x0+radius>=lcdWidth-1))
	{
		lcdError=11;
		return(-1);
	}
	if((y0-radius<0) || (y0 +radius>=lcdHeight-1))
	{
		lcdError=11;
		return(-1);
	}
	s_start=(double)start;
	s_end=(double)end;
	switch(mode)
	{
		case OpenArc : // arc drawn with foreground colour
		case ClosedArc :
			dstep=360.0/(2*radius*M_PI);
			s_end+=dstep;
			for(degree=s_start;degree<=s_end;degree+=dstep)
			{
				radian=deg2rad(degree);
				x=cos(radian)*(double)radius;
				y=sin(radian)*(double)radius;
				pixptr=gptr+lcdOffset((x0+x),(y0-y));
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
			}
			if(mode==OpenArc)//  || (mode==ClosedArc))
				break;
			radian=deg2rad(s_start);
			x=cos(radian)*(double)radius;
			y=sin(radian)*(double)radius;
			x1=x0+x;
			y1=y0-y;
			radian=deg2rad(s_end);
			x=cos(radian)*(double)radius;
			y=sin(radian)*(double)radius;
			x2=x0+x;
			y2=y0-y;
			GraphicsControlFlag |= 1;
			lcddrawLine(x1,y1,x2,y2);
			GraphicsControlFlag ^= 1;
			break;
	}// end of switch mode
	x1=x0-radius;
	y1=y0-radius;
	x2=x0+radius;
	y2=y0+radius;
	bitBlt(x1,y1,x2,y2);
	return 0;
}


byte lcddrawCircle(word x0,word y0,word radius,byte mode)//draws filled or not circle to host memory then to display using foreground colour
{
int x,y,z,dx;
int c;
double radian,degree,dstep;
byte *pixptr;

	if((mode<0) || (mode>1))
	{
		lcdError=12;
		return(-1);
	}
	if((x0<0) || (x0>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y0<0) || (y0>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if((x0-radius<0) || (x0+radius>lcdWidth-1))
	{
		lcdError=13;
		return(-1);
	}
	if((y0-radius<0) || (y0 +radius>lcdHeight-1))
	{
		lcdError=13;
		return(-1);
	}
	switch(mode)
	{
		case 0 : // circle drawn with foreground colour*/
			dstep=360.0/(2*radius*M_PI);
			for(degree=0.0;degree<=90.0;degree+=dstep)
			{
				radian=deg2rad(degree);
				x=cos(radian)*(double)radius;
				y=sin(radian)*(double)radius;
				pixptr=gptr;
				pixptr+=lcdOffset(x0+x,y0-y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
				pixptr=gptr;
				pixptr+=lcdOffset(x0-x,y0-y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
				pixptr=gptr;
				pixptr+=lcdOffset(x0-x,y0+y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
				pixptr=gptr;
				pixptr+=lcdOffset(x0+x,y0+y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
			}
			break;

		case 1 : // filled circle with foreground colour
			for(z=0;z<=radius;++z)
			{
				x=sqrt((pow(radius,2)-pow(z,2)));
				dx=(x0+x)-(x0-x);
				pixptr=gptr;
				pixptr+=lcdOffset(x0-x,y0-z);
				for(c=0;c<=dx;c++)
				{
					*(pixptr+(c*3))=Foreground[0];
					*(pixptr+(c*3)+1)=Foreground[1];
					*(pixptr+(c*3)+2)=Foreground[2];
				}
				pixptr=gptr;
				pixptr+=lcdOffset(x0-x,y0+z);
				for(c=0;c<=dx;c++)
				{
					*(pixptr+(c*3))=Foreground[0];
					*(pixptr+(c*3)+1)=Foreground[1];
					*(pixptr+(c*3)+2)=Foreground[2];
				}
			}
			break;
	} // end of switch mode
	bitBlt(x0-radius,y0-radius,x0+radius,y0+radius);
	return 0;
}

byte lcddrawEllipse(word x0,word y0,word xr,word yr,byte mode)// draws filled or not ellipse to host memory then display
{
int x,y,dx,count;
double radian,degree,dstep;
byte *pixptr;

	if((x0<0) || (x0>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y0<0) || (y0>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if(xr==yr)
		return(lcddrawCircle(x0,y0,xr,mode));
	if((x0-xr)<0 || (x0+xr)>(lcdWidth-1))
	{
		lcdError=14;
		return(-1);
	}
	if((y0-yr)<0 || (y0+yr)>(lcdHeight-1))
	{
		lcdError=15;
		return(-1);
	}
	dstep=360.0/((float)(yr+xr)*2*M_PI);
	switch(mode)
	{
		case(Nofill):
			for(degree=0.0;degree<=90.0;degree+=dstep)
			{
				radian=deg2rad(degree);
				x=cos(radian)*(double)xr;
				y=sin(radian)*(double)yr;
				pixptr=gptr;
				pixptr+=lcdOffset(x0+x,y0-y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
				pixptr=gptr;
				pixptr+=lcdOffset(x0-x,y0-y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
				pixptr=gptr;
				pixptr+=lcdOffset(x0-x,y0+y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
				pixptr=gptr;
				pixptr+=lcdOffset(x0+x,y0+y);
				*pixptr=Foreground[0];
				*(pixptr+1)=Foreground[1];
				*(pixptr+2)=Foreground[2];
			}
			break;
		case(Fill):
			for(degree=0.0;degree<=90.0;degree+=dstep)
			{
				radian=deg2rad(degree);
				x=cos(radian)*(double)xr;
				y=sin(radian)*(double)yr;
				dx=(x0+x)-(x0-x);
				for(count=0;count<=dx;count++)
				{
					pixptr=gptr;
					pixptr+=lcdOffset(x0-x,y0-y);
					*(pixptr+(count*3))=Foreground[0];
					*(pixptr+(count*3)+1)=Foreground[1];
					*(pixptr+(count*3)+2)=Foreground[2];
					pixptr=gptr;
					pixptr+=lcdOffset(x0-x,y0+y);
					*(pixptr+(count*3))=Foreground[0];
					*(pixptr+(count*3)+1)=Foreground[1];
					*(pixptr+(count*3)+2)=Foreground[2];
				}
			}
			break;
	}
	bitBlt(x0-xr,y0-yr,x0+xr,y0+yr);
	return 0;
}


byte lcddrawLine(int x1,int y1,int x2,int y2)//draws lines diagonally as well as horz or vert
{
int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;
unsigned int offset;
byte *pixptr;


	if((x1<0) || (x1>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((x2<0) || (x2>lcdWidth-1))
	{
		lcdError=6;
		return(-1);
	}
	if((y1<0) || (y1>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	if((y2<0) || (y2>lcdHeight-1))
	{
		lcdError=7;
		return(-1);
	}
	dx=x2-x1;
	dy=y2-y1;
	dxabs=abs(dx);
	dyabs=abs(dy);
	if(dx==0) // line is vertical
		return(lcddrawVline(x1,y1,y2));
	if(dy==0) // line is horizontal
		return(lcddrawHline(x1,x2,y1));
	sdx=sgn(dx);
	sdy=sgn(dy);
	x=dyabs>>1;
	y=dxabs>>1;
	px=x1;
	py=y1;
	if(dxabs>=dyabs)
	{
		for(i=0;i<dxabs;i++)
		{
			y+=dyabs;
			if(y>=dxabs)
			{
				y-=dxabs;
				py+=sdy;
			}
			px+=sdx;
			pixptr=gptr;
			offset=lcdOffset(px,py);
			pixptr+=offset;
			*(pixptr)=Foreground[0];
			*(pixptr+1)=Foreground[1];
			*(pixptr+2)=Foreground[2];
		}
	}
	else
	{
		for(i=0;i<dyabs;i++)
		{
			x+=dxabs;
			if(x>=dyabs)
			{
				x-=dyabs;
				px+=sdx;
			}
			py+=sdy;
			pixptr=gptr;
			offset=lcdOffset(px,py);
			pixptr+=offset;
			*(pixptr)=Foreground[0];
			*(pixptr+1)=Foreground[1];
			*(pixptr+2)=Foreground[2];
		}
	}
	if(GraphicsControlFlag & 1)
		return(0);
	bitBlt(x1,y1,x2,y2); //comment out for new above
	return 0;
}

int lcddrawChar(word x,word y,char*k)// draws characters to host memory and display
{
unsigned int index,oset;
unsigned int lin;
byte col;
byte *tptr;
byte attributes = 0;
byte savcolor[3];

	if((FontControlFlag&VERTICAL) && (FontControlFlag&CH_UL))
	{
		lcdError=5;
		return(-1);
	}
	if(x+fontWidth>lcdWidth-1)
	{
		lcdError=2;
		return(-1);
	}
	if(y+fontHeight>lcdHeight-1)
	{
		lcdError=3;
		return(-1);
	}
	index=(*k*fontHeight)+2;
	oset=lcdOffset(x,y);
	lin=2;
	for(col=0;col<3;++col)
	{
		if(FontControlFlag&(lin<<col))
			attributes=FontControlFlag&(lin<<col);
	}
	for(lin=1;lin<=fontHeight;++lin)
	{
		tptr=(gptr + ((lin-1)*scanlinebytes) + oset);					// sas's -3
		for(col=1; col<=fontWidth+1; col ++)						// sas's -1
		{
			switch(attributes)
			{
				case 2 :
					if(((font[index+(lin-1)]<<(col-1))&0b1000000000000000))
					{ /// normal
						*(tptr+((col-1)*3))=Textcolor[0];
						*(tptr+((col-1)*3)+1)=Textcolor[1];
						*(tptr+((col-1)*3)+2)=Textcolor[2];
					}
					break;
				case 4 :
					if(((font[index+(lin-1)]<<(col-1))&0b1000000000000000))
					{ /// foreground
						*(tptr+((col-1)*3))=Textcolor[0];
						*(tptr+((col-1)*3)+1)=Textcolor[1];
						*(tptr+((col-1)*3)+2)=Textcolor[2];
					}
					else
					{ /// background
						*(tptr+((col-1)*3))=TextBkcolor[0];
						*(tptr+((col-1)*3)+1)=TextBkcolor[1];
						*(tptr+((col-1)*3)+2)=TextBkcolor[2];
					}
					break;
				case 8 :
					if(((font[index+(lin-1)]<<(col-1))&0b1000000000000000))
					{ /// foreground
						*(tptr+((col-1)*3))=TextBkcolor[0];
						*(tptr+((col-1)*3)+1)=TextBkcolor[1];
						*(tptr+((col-1)*3)+2)=TextBkcolor[2];
					}
					else
					{ /// background
						*(tptr+((col-1)*3))=Textcolor[0];
						*(tptr+((col-1)*3)+1)=Textcolor[1];
						*(tptr+((col-1)*3)+2)=Textcolor[2];
					}
					break;
			} // end switch attributes
		}//end for col
		if(!(FontControlFlag&1))
		{
			if(FontControlFlag&16)
			{
				savcolor[0]=Foreground[0];
				savcolor[1]=Foreground[1];
				savcolor[2]=Foreground[2];
				Foreground[0]=Textcolor[0];
				Foreground[1]=Textcolor[1];
				Foreground[2]=Textcolor[2];
				lcddrawHline(x,x+fontWidth,y+fontHeight);
				Foreground[0]=savcolor[0];
				Foreground[1]=savcolor[1];
				Foreground[2]=savcolor[2];
			}
		}
	}// end for lin
	if(GraphicsControlFlag&1)
		return(0);
	else
		bitBlt(x,y,x+fontWidth+1,y+fontHeight);
	return(0);
}


word stringPixels(char* str)// calculates required horz / vert pixels for given string length and font
{
	if(!(FontControlFlag&1))
		return(strlen(str)*fontWidth);
	else
		return(strlen(str)*fontHeight);
}

int lcddrawString(int x,int y,char * strptr)// draws string to host and display if bit one of graphicscontrolflag is ON then char above draws only to host
{                                                          // and this function outputs all data at once when string is done
int length,index;

	if((FontControlFlag&VERTICAL) & (FontControlFlag&CH_UL))
	{
		lcdError=5;
		return(-1);
	}
	length=stringPixels(strptr);
	if(!(FontControlFlag&1))//direction is horizontal
	{
		if(x+length>lcdWidth-1)
		{
			lcdError=4;
			return(-1);
		}
		if(y+fontHeight>lcdHeight-1)
		{
			lcdError=4;
			return(-1);
		}
	}
	else // direction is vertical
	{
		if(x+fontWidth>=lcdWidth-1)
		{
			lcdError=2;
			return(-1);
		}
		if(y+length>lcdHeight-1)
		{
			lcdError=4;
			return(-1);
		}
	}
	GraphicsControlFlag |= BITBLIT;
	length=strlen(strptr);
	for(index=0;index<length;++index)
	{
		switch((FontControlFlag&1))
		{
			case HORIZONTAL :
				lcddrawChar(x+(index*fontWidth),y,(strptr+index));
				break;
			case VERTICAL :
				lcddrawChar(x,y+(index*fontHeight),(strptr+index));
				break;
			default :
				break;
		}
	}

	GraphicsControlFlag ^= BITBLIT;
	switch((FontControlFlag&1))
	{
		case HORIZONTAL :
			bitBlt(x,y,x+(fontWidth*length),y+fontHeight);
			break;
		case VERTICAL :
			bitBlt(x,y,x+fontWidth,y+(fontHeight*length));
			break;
		default :
			break;
	}
	return(0);
}

int lcddrawTriangle(word x0,word y0,word radius,byte type,word ang1,word ang2,word ang3)
{

	return(0);
	lcddrawLine(triangle[0][0],triangle[0][1],triangle[1][0],triangle[1][1]);
	lcddrawLine(triangle[1][0],triangle[1][1],triangle[2][0],triangle[2][1]);
	lcddrawLine(triangle[2][0],triangle[2][1],triangle[0][0],triangle[0][1]);
	//return(0);
}

byte lcdOpenFont(byte index)
{
FILE *fontFD;
char fontPath[255];
char fontName[24];
word len,fontsize;

	if((index<0) || (index>8))
	{
		lcdError=16;
		printf("Invalid Font Index\n");
		return(1);
	}
	strcpy(fontPath,"");
	strcpy(fontPath,fPath);
	strcpy(fontName,FontNames[index]);
	len=strlen(fontName);
	while(fontName[len]!='x')
		len--;
	len++;
	fontName[len+2]=0x0;
	fontsize=atoi(&fontName[len]);
	fontsize=fontsize*2*256+2;
	strcat(fontPath,FontNames[index]);
	fontFD=fopen(fontPath,"r");
	if(fontFD==NULL)
	{
		printf("Unable to load font %s file. Aborting\n",fontPath);
		printf("%s\n",strerror(errno));
		lcdError=17;
		return(1);
	}
	memset(font,0,(sizeof(font)));
	fread(font,sizeof(byte),fontsize*sizeof(short int),fontFD);//needs some fixing for differing font sizes
	fclose(fontFD);
	fontFD=NULL;
	fontWidth=font[0];
	fontHeight=font[1];
	return(0);
}

void lcdCloseDisplay(void)
{
	EXIT(0);
}

void lcdOpenDisplay(byte fontdex)
{
int bytesRead,z;
char initPath[255];
struct display_data
{
	word init_size;
	word width;
	word height;
}display_data;
short int init_fd;

	gpioOpen();//opens file to gpiomem
	gpioPins();// uses file fd above to associate file and 'Broadcom pin number,direction,and intial values
	printf("Gpio pins initialized.\n");
	strcpy(initPath,"");
	strcpy(initPath,"/home/pi/IliTek_Displays/Init_Code/");
	strcat(initPath,iliINIT[iliINITindex]);
	printf("Opening init file %s\n",iliINIT[iliINITindex]);
	memset(iliINITcode,0x0,sizeof(iliINITcode));
	init_fd=open(initPath,O_RDONLY);
	if(init_fd<0)
	{
		printf("Fatal! Failed to open %s  Aborting\n",iliINIT[iliINITindex]);
		EXIT(1);
	}
	bytesRead=read(init_fd,&display_data,sizeof(display_data));
	if(bytesRead<6)
	{
		printf("Fatal! Failed to Read %s  Aborting\n",iliINIT[iliINITindex]);
		EXIT(1);
	}
	z=display_data.init_size;
	bytesRead=read(init_fd,iliINITcode,z);
	close(init_fd);
	if(bytesRead<z)
	{
		printf("Fatal! Failed to Read init code. Aborting %s\n",iliINIT[iliINITindex]);
		EXIT(1);
	}
	INITBYTES=display_data.init_size;
	lcdWidth=display_data.width;
	lcdHeight=display_data.height;
	gptr=malloc(lcdWidth*lcdHeight*3);// 18 bit color ie 3 bytes per pixel HOST memory buffer
	if(gptr==NULL)
	{
		printf("Unable to allocate Write buffer. Aborting\n");
		EXIT(1);
	}
	GPTR=gptr;
	Rxptr=malloc(4096);// spi duplex recieve buffer
	if(Rxptr==NULL)
	{
		printf("Unable to allocate Read buffer. Aborting\n");
		EXIT(1);
	}
	RXPTR=Rxptr;
	memset(gptr,0,lcdWidth*lcdHeight*3);
	memset(Rxptr,0xff,4096);
	lcdFD=SPISetup (SPI_NAME,SPIspeed); //opens spidevX.X
	if(lcdFD<0)
	{
		printf("Failed to open SPI! Aborting\n");
		EXIT(1);
	}
	printf("%s Display opened  %d X  %d\n",iliChips[iliINITindex],lcdWidth,lcdHeight);
	printf("%s opened for Display @  %d Mhz\n",spiDevice[SPI_index],SPIspeed);
	scanlinebytes=lcdWidth*3;
	bytesperscreen=scanlinebytes*lcdHeight;
	lcdOpenFont(fontdex);
	kbinit();// sets up console for non-blocking keyboard reads
	if(!KBinitialized)
	{
		printf("Failed to initialize Keyboard  Aborting\n");
		EXIT(1);
	}
	printf("Initializing the display\n");
	hardReset();
	lcdinitPanel();
	if(GraphicsControlFlag &0x80) // reset orientation here if command line parameter changes default
	{
		lcdBGR=bgr;
		lcdOrientation(rotateAngle);
	}
	GraphicsControlFlag=0;
	FontControlFlag=2;
	color18(Background,0xffffff);
	color18(Foreground,0xff0000);
	color18(Textcolor,0x0000ff);
	color18(TextBkcolor,0xff0000);
	return;
}

//************************ End functions *******************************
/*
int main(int argc,char *argv[])
{
word z;
clock_t start,end;
float time;
char ch[2];
int x,y,lin,ccount;
char teststr[40];
byte *savptr;
//byte *pixptr;





	strcpy(SPI_NAME,"SPI1_2");// set default spi bus/channel to use
	SPIspeed=32;//               set default bus speed
	GraphicsControlFlag=0;//     write to host buffer and display
	FontControlFlag=2;//         character text colour only NO background
	if(argc>1)
	{
		memset(initList,255,sizeof(initList));
		initParamCount=argc;
		for(x=1;x<initParamCount;++x)
		{
			if((strcasecmp(argv[x],iliParams[x])==0))
				initList[x]=0;

		}
		x=1;
		while(x<(initParamCount-1))
		{
			if(initList[x]==0)
			{
				for(z=0;z<ILIparams;++z)
				{
					if((strcasecmp(argv[x],"DISPLAY")==0))
					{
						initList[x]=255;
						x+=2;
						for(z=0;z<iliLCDs;++z)
						{
							if((strcasecmp(argv[x+1],iliChips[z])==0))
								iliINITindex=z;
						}
						goto LOOP;
					}
				}
				for(z=0;z<ILIparams;++z)
				{
					if((strcasecmp(argv[x],"DC")==0))
					{
						DC=atoi(argv[z+x+1]);
						initList[x]=255;
						x+=2;
						goto LOOP;
					}
					if((strcasecmp(argv[x],"RESET")==0))
					{
						RESET=atoi(argv[z+x+1]);
						initList[x]=255;
						x+=2;
						goto LOOP;
					}
					if((strcasecmp(argv[x],"LED")==0))
					{
						LED=atoi(argv[z+x+1]);
						initList[x]=255;
						x+=2;
						goto LOOP;
					}
					if((strcasecmp(argv[x],"BGR")==0))
					{
						bgr=atoi(argv[z+x+1]);
						initList[x]=255;
						GraphicsControlFlag |= 0x80;// set a flag bit so lcdopen will make a call to set rotation and or RGB
						x+=2;
						goto LOOP;
					}
					if((strcasecmp(argv[x],"SPI")==0))
					{

						if(strlen(argv[z+x+1])<=9)
						{
							strcpy(SPI_NAME,argv[z+x+1]);
							if(isdigit(argv[z+x+2]))
								SPIspeed=atoi(argv[z+x+2]);
							initList[x]=255;
						}
						x+=3;
						goto LOOP;
					}
					if((strcasecmp(argv[x],"ROTATE")==0))
					{
						rotateAngle=atoi(argv[x+1]);
						GraphicsControlFlag |= 0x80;
						initList[x]=255;
						x+=2;
						goto LOOP;
					}// end if ROTATE argument;
					if(((strcasecmp(argv[x],"?")==0)) || ((strcasecmp(argv[x],"H")==0)))
					{
						printf("Init file to use ie.Ili9431,Ili9486 etc.\n");
						printf("All pins use the Broadcom pin number ie. GPIO(18) only use the number.\n");
						printf("DC [Pin number] must include the space(s) they are delimiters.\n");
						printf("RESET [Pin number]\n");
						printf("LED [Pin number]\n");
						printf("ROTATE [Pin number]\n");
						printf("? or H displays these messages.\n");
						printf("Ordering and capitalization is unimportant.\n");
						EXIT(0);
					}
				}
			}
			else
				++x;
			LOOP:;
		}// end parameter loop
		printf("Display       %s\n",iliChips[iliINITindex]);
		printf("DC    GPIO =  %d\n",DC);
		printf("RESET GPIO =  %d\n",RESET);
		printf("LED   GPIO =  %d\n",LED);
		printf("RGB     =     %d\n",bgr);
		printf("Rotation      %d\n",ROTATE);
		printf("Default SPI   %s\n",SPI_NAME);
		printf("Default speed %d\n",SPIspeed*1000000);
	}//end if args
	else
	{
		printf("Default display       %s\n",iliChips[iliINITindex]);
		printf("Default DC    GPIO =  %d\n",DC);
		printf("Default RESET GPIO =  %d\n",RESET);
		printf("Default LED   GPIO =  %d\n",LED);
		printf("RGB         =         %d\n",bgr);
		printf("Default Rotation      %d\n",ROTATE);
		printf("Default SPI           %s\n",SPI_NAME);
		printf("Default speed %d\n",SPIspeed*1000000);
		printf("Proceed using default values Y/N + [enter]\n");

		while(1)
		{
			ch[0]=getchar();
			if((ch[0]=='y') || (ch[0]=='Y'))
				break;
			if((ch[0]=='n') || (ch[0]=='N'))
			{
				printf("After program name [SPACE] then ? or h or H for help to override defaults.\n");
				EXIT(0);
			}
		}
	}//end all to do with program arguments

	printf("Initializing the display and hardware\n");
	lcdOpenDisplay(f12x27);
	lcdon();
	GraphicsControlFlag |= 0x40;
	for(x=0;x<2;++x)
	{
		lcdShowBmp(Bmp_Demo[x]);
		printf("Press q to quit or b to toggle backlight any other key to continue.\n\n");
		while(!lcdChk_Key())
			;
	}
	ch[1]=0x0;
	ch[0]='A';
	strcpy(teststr,"Hello World :-)");
	printf("Clearing the display\n");
	start=clock();
	lcdClear();
	end=clock();
	lcdon();
	time=((float)end-start)/CLOCKS_PER_SEC;
	printf("Time to clear display %.6f mS\n",time*1000);
	printf("Frquency       =      %.2f  Hz\n\n",1.0/time);
	printf("Drawing a single character.\n");
	lcddrawChar((lcdWidth/2)-(fontWidth/2),(lcdHeight/2)-(fontHeight/2),ch);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	start=clock();
	printf("Draw 4 horizontal strings differing attributes.\n");
	lcddrawString(5,10,teststr);
	FontControlFlag=0;
	FontControlFlag |= CH_BG;
	lcddrawString(5,10+(fontHeight*2),teststr);
	FontControlFlag=0;
	FontControlFlag |= CH_INV;
	lcddrawString(5,10+(fontHeight*4),teststr);
	FontControlFlag=0;
	FontControlFlag |= CH_UL;
	FontControlFlag |= CH_ONLY;
	lcddrawString(5,10+(fontHeight*6),teststr);
	FontControlFlag=3;
	end=clock();
	time=((float)end-start)/CLOCKS_PER_SEC;
	printf("Time to draw strings  %.6f mS\n",time*1000);
	printf("Frquency       =      %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	strcpy(teststr,"Bunny");
	printf("Drawing a vertical string\n");
	lcddrawString(lcdWidth-(fontWidth*2),10,teststr);
	FontControlFlag=2;
	printf("Press q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	color18(Foreground,0xff0000);
	printf("Clearing the display\n");
	lcdClear();
	//lcdOrientation(0);// ORIENTATION CHANGE 0 , 90 , 180 , 270
	GraphicsControlFlag=0;
	printf("Drawing a horizontal line.\n");
	start=clock();
	lcddrawHline(0,lcdWidth-1,lcdHeight/2);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to draw horizontal line            %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("Drawing a vertical line.\n");
	start=clock();
	lcddrawVline(lcdWidth/2,0,lcdHeight-1);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to draw vertical line              %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Drawing a filled rectangle\n");
	start=clock();
	lcddrawRectangle(50,lcdHeight/2-50,100,lcdHeight/2+50,Fill);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to draw filled rectangle           %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Drawing a another rectangle to edge of display\n");
	lcddrawRectangle(lcdWidth-50,lcdHeight/2,lcdWidth-1,lcdHeight/2+50,Nofill);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Saving rectangle prior to being painted'\n");
	start=clock();
	savptr=lcdsaveArea(50,lcdHeight/2-50,100,lcdHeight/2+50);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to save filled rectangle           %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	color18(Foreground,0xff00ff);
	printf("Painting previous rectangle.\n");
	lcddrawRectangle(50,lcdHeight/2-50,100,lcdHeight/2+50,Fill);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	if(savptr!=NULL)
	{
		printf("Restoring previous rectangle color.\n");
		lcdrestoreArea(50,lcdHeight/2-50,savptr);
		free(savptr);
	}
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Clearing previous rectangle.\n");
	lcdclearArea(50,lcdHeight/2-50,100,lcdHeight/2+50);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Invert ON\n");
	lcdInvert(ON);
	printf("Press q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Invert OFF\n");
	lcdInvert(OFF);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Clearing the display\n");
	lcdClear();
	printf("Draw circle\n");
	start=clock();
	lcddrawCircle(lcdWidth/2,lcdHeight/2,100,Nofill);
	end=clock();
	time=((float)end-start)/CLOCKS_PER_SEC;
	printf("Time to display  circle       =         %.6f mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	color18(Foreground,0xff0000);
	printf("Draw diagonal lines\n");
	start=clock();
	lcddrawLine(0,0,lcdWidth-1,lcdHeight-1);
	lcddrawLine(0,lcdHeight-1,lcdWidth-1,0);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to draw diagonal lines             %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Clearing the display\n");
	lcdClear();
	color18(Foreground,0xff00ff);
	printf("Draw ellipse\n");
	start=clock();
	lcddrawEllipse(lcdWidth/2,lcdHeight/2,(lcdWidth/4),(lcdHeight/2-2),Nofill);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to display ellipse 200 x 310       %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Drawing horizontal with pixels.\n");
	start=clock();
	for(x=0;x<=lcdWidth-1;++x)
		lcdPutpixel(x,lcdHeight/2);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to display line with lcdPutPixel   %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Clearing the display\n");
	lcdClear();
	printf("Drawing arc of any angle may be open or closed no fill at this time.\n\n");
	start=clock();
	lcddrawArc(lcdWidth/2,lcdHeight/2,30,240,100,ClosedArc);
	end=clock();
	time=((float)(end-start))/CLOCKS_PER_SEC;
	printf("Time to draw close arc                  %.2f  mS\n",time*1000);
	printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	printf("Clearing the display\n");
	GraphicsControlFlag |= BITBLIT;
	for(y=0;y<8;++y)
	{
		lcdOpenFont(y);
		lcdClear();
		ccount=0;
		lin=10;
		x=0;
		start=clock();
		while(ccount<256)
		{
			lcddrawChar(x,lin,(char*)&ccount);
			++ccount;
			x+=fontWidth;
			if(x>=lcdWidth)
			{
				x=0;
				lin+=fontHeight;
				if(lin+fontHeight>lcdHeight-1)
				{
					bitBlt(0,0,lcdWidth-1,lcdHeight-1);
					printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
					while(!lcdChk_Key())
						;
					lin=10;
					x=0;
					printf("Clearing the display\n");
					lcdClear();
				}
			}
		}
		printf("Refreshing the display\n");
		if(x>0)
		{
			lcdRefresh();
			end=clock();
			printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
			while(!lcdChk_Key())
				;
		}
		else
			end=clock();
		time=((float)end-start)/CLOCKS_PER_SEC;
		printf("Time to draw character set      =       %.6f mS\n",time*1000);
		printf("Frquency                =               %.2f  Hz\n\n",1.0/time);
	}
	GraphicsControlFlag ^= BITBLIT;
	printf("Pixel color is %06x\n",lcdGetpixel(lcdWidth/2,lcdHeight-1));
	printf("BMP image Demo\n");
	for(x=2;x<BITmapIMAGES;++x)
	{
		lcdShowBmp(Bmp_Demo[x]);
		printf("Press q to quit or b to toggle backlight any other key to continue.\n\n");
		while(!lcdChk_Key())
			;
	};
	printf("\nPress q to quit or b to toggle backlight any other key to continue.\n\n");
	while(!lcdChk_Key())
		;
	lcdCloseDisplay();
	return(0);
}
*/
