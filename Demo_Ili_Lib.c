#include "/home/pi/IliTek_Displays/Includes/ILI_SPI.h"
#include "/home/pi/IliTek_Displays/Includes/extrafunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>



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

int main(int argc,char *argv[])
{
word z;
clock_t start,end;
float time;
char ch[2];
int x,y,lin,ccount,listx;
char teststr[40];
byte *savptr;
//byte *pixptr;
byte initList[ILIparams];
word initParamCount=0;




	// printf("argc = %d\n",argc);
	if(argc>1)
	{
		for(x=1;x<argc;++x)
		{
			strcpy(ARGS[x],argv[x]);
		}
		memset(initList,255,sizeof(initList));
		initParamCount=argc;
		for(x=1;x<initParamCount;++x)
		{
			for(z=1;z<ILIparams;++z)
			{
				if((strcasecmp(ARGS[x],iliParams[z])==0))
				{
					initList[z]=0;
				}
			}
		}
		x=1;
		ccount=1;
		while(x<(initParamCount))
		{
			for(listx=1;listx<ILIparams;++listx)
			{
				if(initList[listx]==0)
				{
					for(z=1;z<ILIparams;++z)
					{
						if((strcasecmp(ARGS[x],"DISPLAY")==0))
						{
							printf("Changing display\n");
							initList[x]=255;
							x+=2;
							for(z=0;z<iliLCDs;++z)
							{
								if((strcasecmp(ARGS[x+1],iliChips[z])==0))
									iliINITindex=z;
							}
							goto LOOP;
						}

						if((strcasecmp(ARGS[x],"DC")==0))
						{
							printf("Changing DC\n");
							DC=atoi(ARGS[z+x+1]);
							initList[x]=255;
							x+=2;
							goto LOOP;
						}
						if((strcasecmp(argv[x],"RESET")==0))
						{
							printf("Changing Reset\n");
							RESET=atoi(ARGS[z+x+1]);
							initList[x]=255;
							x+=2;
							goto LOOP;
						}
						if((strcasecmp(ARGS[x],"LED")==0))
						{
							printf("Changing Led\n");
							LED=atoi(ARGS[z+x+1]);
							initList[x]=255;
							x+=2;
							goto LOOP;
						}
						if((strcasecmp(ARGS[x],"BGR")==0))
						{
							printf("Changing BGR");
							bgr=atoi(ARGS[z+x+1]);
							initList[x]=255;
							GraphicsControlFlag |= 0x80;// set a flag bit so lcdopen will make a call to set rotation and or RGB
							x+=2;
							goto LOOP;
						}
						if((strcasecmp(ARGS[x],"SPI")==0))
						{
							if(strlen(ARGS[z+x+1])<=9)
							{
								strcpy(SPI_NAME,ARGS[z+x+1]);
								if(isdigit(ARGS[z+x+2]))
									SPIspeed=atoi(ARGS[z+x+2]);
								initList[x]=255;
							}
							x+=3;
							goto LOOP;
						}
						if((strcasecmp(ARGS[x],"ROTATE")==0))
						{
							printf("Changing rotation\n");
							rotateAngle=atoi(ARGS[x+1]);
							GraphicsControlFlag |= 0x80;
							initList[x]=255;
							x+=2;
							goto LOOP;
						}// end if ROTATE argument;
						if(((strcasecmp(ARGS[x],"?")==0)) || ((strcasecmp(ARGS[x],"H")==0)))
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
					}// end of scan for loop
				}//end if initList
				//else
				//	++x;
				++ccount;
			}//end forst for loop for scan

			LOOP:;
		}// end while parameter loop
		printf("\n\nInitializing with these values.\n\n");
		printf("Display       %s\n",iliChips[iliINITindex]);
		printf("DC    GPIO =  %d\n",DC);
		printf("RESET GPIO =  %d\n",RESET);
		printf("LED   GPIO =  %d\n",LED);
		printf("RGB     =     %d\n",bgr);
		printf("Rotation      %d\n",rotateAngle);
		printf("Default SPI   %s\n",SPI_NAME);
		printf("Default speed %d\n\n",SPIspeed*1000000);
	}//end if args
	else
	{
		printf("\n\nDefault display       %s\n",iliChips[iliINITindex]);
		printf("Default DC    GPIO =  %d\n",DC);
		printf("Default RESET GPIO =  %d\n",RESET);
		printf("Default LED   GPIO =  %d\n",LED);
		printf("RGB         =         %d\n",bgr);
		printf("Default Rotation      %d\n",ROTATE);
		printf("Default SPI           %s\n",SPI_NAME);
		printf("Default speed %d\n",SPIspeed*1000000);
		printf("Proceed using default values Y/N + [enter]\n\n");

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

//******************* Your code starts below *******************

	lcdOpenDisplay(f12x27);
	lcdon();// Turn on backlight off by default
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

