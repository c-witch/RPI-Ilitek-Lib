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
#if!defined byte
typedef unsigned char byte;
#endif
#if!defined word
typedef unsigned short int word;
#endif

#define f09x16 0
#define f12x16 1
#define f10x20 2
#define f12x23 3
#define f12x24 4
#define f12x27 5
#define f14x30 6
#define f16x32 7
extern word fontWidth;
extern word fontHeight;



#define Fill 1
#define Nofill 0
//#define Degree 248 // character code for degree symbol
#define OpenArc 0
#define ClosedArc 1


#if!defined BITBLIT
#define BITBLIT 1
#endif

extern word ROTATE;
extern word rotateAngle;
extern byte displayRotate;

//************************ SPI data below ******************************

extern char SPI_NAME[15];
extern int SPIspeed;

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
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
};
typedef char names[30];
names ARGS[20];

extern byte bgr;
extern byte Foreground[3],Background[3],Textcolor[3],TextBkcolor[3];
extern word lcdWidth;
extern word lcdHeight;
extern byte FontControlFlag;
extern byte GraphicsControlFlag;
#if!defined iliLCDs
#define iliLCDs 4
#endif
extern char *iliChips[iliLCDs];
extern word iliINITindex;

extern word DC;
extern word RESET;
extern word LED;





//************************* Proto Types ********************************
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
