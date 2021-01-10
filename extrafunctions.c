//#include "/home/pi/Desktop/Cee/Cee/Local_Includes/extrafunctions.h"
#include <stdio.h>
#include <stddef.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <math.h>
#include <string.h>

int KBinitialized;
struct termios original_tty,tty;





double deg2rad(double degrees)
{
	return((degrees/180.0)*M_PI);
}

double rad2deg(double radians)
{
	return((radians/M_PI)*180.0);
}


// call this during main initialization
// note kbhit will call this if it hasnt been done yet
void kbinit()
{
	//struct termios tty;
	tcgetattr(fileno(stdin),&original_tty);
	tty=original_tty;
	// disable ICANON line buffering and echo
	tty.c_lflag&= ~ICANON;
	tty.c_lflag&= ~ECHO;
	tcsetattr(fileno(stdin),TCSANOW,&tty);
	setbuf(stdin,NULL);
	KBinitialized=1;
}

int kbhit()
{
	if(!KBinitialized)
	{
		kbinit();
	}
	int bytesWaiting;
	ioctl(fileno(stdin),FIONREAD,&bytesWaiting);
	return(bytesWaiting);
}

void kbfini()
{
	if(KBinitialized)
	{
		tcsetattr(fileno(stdin),TCSANOW,&original_tty);
		KBinitialized=0;
	}
}

int substr(char str[],char substr[]) // add this to my extrafunctions library
{
int length,length1,p,x,y;


	if(strlen(substr)>=strlen(str))
		return -1;
	length=strlen(str);
	for(x=0;x<length;++x)
	{
		p=0;
		if((str[x]==substr[0]))
		{

			p=x;
			length1=strlen(substr);
			y=0;
			while((str[p])==(substr[y]) && (y<length1))
			{
				//printf("%c  %c\n",str[p],substr[y]);
				++p;
				++y;
				if(y==length1)
					return p;
			}
		}
	}
	return 0;
}
