// ES-exercise 06                                                            //
// Demo to initialize TFT-Display with ST7735R controller,                   //
// e.g. joy-it RB-TFT1.8-V2.                  //
// configuration:  4-line serial interface, RGB-order: R-G-B,        //

// SRAM usage
// 1kb for various variables
// 95kb for framebuffer  -> max 47500 pixels with 16bit data each
// 128x160 display area       = 20480 pixels

// 8bit 332-RGB format

#include <SPI.h>
//#include "stdint.h"
//#include "math.h"
//#include "string.h"
//#include "strutils.h"
#define _BV(n) (1 << n)

#include <DueTimer.h>
DueTimer Timer3;
 
//pin declarations
#define TFT_CS     10   //display: CS-pin
#define TFT_RST     9   //display: reset
#define TFT_DC      8   //display: Data/Command (D/C)

#if defined(__AVR_ATmega2560__)
 #define SS_SLAVE   53  
 //must be put into output mode; otherwise ATmega could assume 
 //to be set into slave mode but SPI-lib doesn't support this
 //mode. So it breaks SPI-lib. Configured as output, the pin 
 //can be used as normal output.
#endif



#define TFT_DC_HIGH()           digitalWrite(TFT_DC, HIGH)
#define TFT_DC_LOW()            digitalWrite(TFT_DC, LOW)
#define TFT_CS_HIGH()           digitalWrite(TFT_CS, HIGH)
#define TFT_CS_LOW()            digitalWrite(TFT_CS, LOW)


//SPI-Settings
#define SPI_DEFAULT_FREQ   1e6      ///< Default SPI data clock frequency
#define SPI_MODE_FALLING_SLOPE SPI_MODE0
SPISettings settingsTFT(SPI_DEFAULT_FREQ, MSBFIRST, SPI_MODE_FALLING_SLOPE);


//TFT-area of 128 x 160 (1.8") TFT
#define FRAMEBUFW 128
#define FRAMEBUFH 160
#define FIRST_COL 2
#define FIRST_ROW 1
#define LAST_COL (FIRST_COL+FRAMEBUFW-1)
#define LAST_ROW (FIRST_ROW+FRAMEBUFH-1)

typedef uint8_t rgb332_t;
rgb332_t framebuf[FRAMEBUFW][FRAMEBUFH];

//TFT's commands
const uint8_t NOP = 0x00;               // no Operation 
const uint8_t SWRESET = 0x01;           // Software reset                                                                                                                  
const uint8_t SLPOUT = 0x11;            //Sleep out & booster on                                                                                                           
const uint8_t DISPOFF = 0x28;           //Display off                                                                                                                          
const uint8_t DISPON = 0x29;            //Display on                                                                                                                       
const uint8_t CASET = 0x2A;             //Column adress set                                                                                                        
const uint8_t RASET = 0x2B;             //Row adress set                                                                                                           
const uint8_t RAMWR = 0x2C;             //Memory write                                                                                                             
const uint8_t MADCTL = 0x36;            //Memory Data Access Control                                                                                                       
const uint8_t COLMOD = 0x3A;            //RGB-format, 12/16/18bit                                                                                                          
const uint8_t INVOFF = 0x20;            // Display inversion off                                                                                                           
const uint8_t INVON = 0x21;             // Display inversion on                                                                                                    
const uint8_t INVCTR = 0xB4;            //Display Inversion mode control                                                                                                   
const uint8_t NORON = 0x13;             //Partial off (Normal)                                                                                                     
                                                                                                                                                                           
const uint8_t PWCTR1 = 0xC0;            //Power Control 1                                                                                                                  
const uint8_t PWCTR2 = 0xC1;            //Power Control 2                                                                                                                  
const uint8_t PWCTR3 = 0xC2;            //Power Control 3                                                                                                                  
const uint8_t PWCTR4 = 0xC3;            //Power Control 4                                                                                                                  
const uint8_t PWCTR5 = 0xC4;            //Power Control 5
const uint8_t VMCTR1 = 0xC5;            //VCOM Voltage setting


//global variables
uint8_t invState = 0;


void TFTwriteCommand(uint8_t cmd){
    TFT_DC_LOW();
     SPI.transfer(cmd);
    TFT_DC_HIGH();
}

void TFTwrite_saCommand(uint8_t cmd){
  SPI.beginTransaction(settingsTFT);
  TFT_CS_LOW();
    TFTwriteCommand(cmd);
  TFT_CS_HIGH();
  SPI.endTransaction();
}

void TFTwriteWindow(uint8_t xs, uint8_t xe, uint8_t ys, uint8_t ye) {
        //test wether parameters stay within address ranges;  should be implemented//
        TFTwriteCommand(NOP);   //normally not neccessary; but if not, the first command after eg. SD-access will be ignored (here: CASET)
        TFTwriteCommand(CASET);
        SPI.transfer(0x00); SPI.transfer(xs);
        SPI.transfer(0x00); SPI.transfer(xe);
        TFTwriteCommand(RASET);
        SPI.transfer(0x00); SPI.transfer(ys);
        SPI.transfer(0x00); SPI.transfer(ye);
}

void TFTinit(void) {
      //power-on-reset of Display
      digitalWrite(TFT_RST, HIGH);
      delay(100);
      digitalWrite(TFT_RST, LOW);
      delay(100);
      digitalWrite(TFT_RST, HIGH);
      delay(100);

      //minimal configuration: only settings which are different from Reset Default Value
      //or not affected by HW/SW-reset
      SPI.beginTransaction(settingsTFT);
      TFT_CS_LOW();

        TFTwriteCommand(SWRESET);
        delay(120);                                     //mandatory delay
        TFTwriteCommand(SLPOUT);        //turn off sleep mode.
        delay(120);
        TFTwriteCommand(PWCTR1);
         SPI.transfer(0xA2);
         SPI.transfer(0x02);
         SPI.transfer(0x84);
        TFTwriteCommand(PWCTR4);
         SPI.transfer(0x8A);
         SPI.transfer(0x2A);
        TFTwriteCommand(PWCTR5);
         SPI.transfer(0x8A);
         SPI.transfer(0xEE);
        TFTwriteCommand(VMCTR1);
         SPI.transfer(0x0E);                    //VCOM = -0.775V

        //Memory Data Access Control D7/D6/D5/D4 / D3 /D2/D1/D0
        //                           MY/MX/MV/ML / RGB/MH/- /-
        // MY- Row Address Order;    ‘0’ =Increment, (Top to Bottom)
        // MX- Column Address Order; ‘0’ =Increment, (Left to Right)
        // MV- Row/Column Exchange;  '0’ = Normal,
        // ML- Scan Address Order;   ‘0’ =Decrement,(LCD refresh Top to Bottom)
        //RGB -                      '0' = RGB color fill order (according to note above '1` corresponds to R-G-B)
        // MH -                      '0' = LCD horizontal refresh left to right
        TFTwriteCommand(MADCTL);
         SPI.transfer(0xC8);

        //RGB-format
        TFTwriteCommand(COLMOD);        //color mode
         SPI.transfer(0x55); //16-bit/pixel; high nibble don't care

        TFTwriteCommand(CASET);
         SPI.transfer(0x00); SPI.transfer(FIRST_COL);
         SPI.transfer(0x00); SPI.transfer(LAST_COL);
        TFTwriteCommand(RASET);
         SPI.transfer(0x00); SPI.transfer(FIRST_ROW);
         SPI.transfer(0x00); SPI.transfer(LAST_ROW);

        TFTwriteCommand(NORON);
        TFTwriteCommand(DISPON);

      TFT_CS_HIGH();
      SPI.endTransaction();
}

rgb332_t bg = 0x00;
rgb332_t fg = 0xEE;

void setup() {
    // set pin-modes
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    #if defined(__AVR_ATmega2560__)  
       pinMode(SS_SLAVE, OUTPUT); 
    #endif
  
    // set inactive levels
    digitalWrite(TFT_RST, HIGH);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, HIGH);
  
    // initialize serial port 0
    Serial.begin(9600);
    Serial.println("Exercise init TFT template\n");
  
    // initialize SPI:
    // several devices: multiple SPI.begin(nn_CS) possible
    SPI.begin();
    delay(10);
  
    TFTinit();
    Serial.println("Display Initialized");
    delay(100);
   
    //clear display
        for (int x = 0; x < FRAMEBUFW; ++x)
        for (int y = 0; y < FRAMEBUFH; ++y)
          setpixel(x, y, bg);
          
    uint16_t time = millis();
        flushframe();
    time = millis() - time;
    Serial.print("time consumption of clear-display: "); Serial.print(time, DEC); Serial.println(" ms");
          
    Serial.println("\nSetup finished\n");

    void on_timer();
    Timer3.configure_duration(5, on_timer);
    //Timer3.configure(10, on_timer);
    Timer3.start();
}

int b = 1;
int redraw = 0;
int rot_dir = 0;
int rotatingbarenabled = false;
void on_timer()
{
    if (rotatingbarenabled)
    {
        redraw = 1;
    }
    else
        if (b = !b)
            StudentIdDemo("Merlin Brandt", "1234567");
        else
            StudentIdDemo("Jannes Noack", "7393842");
}

void loop() 
{
    if (redraw)
    {
      rot_bar(25);
      redraw = 0;
    }
    //animation(fg);
    //animation(bg);  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ANIMATIONS

void StudentIdDemo(char const *student, char const *matrikelnr)
{
    clear(bg);
    printString(10, 10, student, fg, bg);
    printString(10, 23, matrikelnr, fg, bg);
    flushframe();
}

void animation(rgb332_t c)
{
    for (int y = 0; y < FRAMEBUFH; ++y)
    {
        for (int x = 0; x < FRAMEBUFW; ++x)
          setpixel(x, y, c);
        flushframe();
        delay(20);
    }
}


void drawline(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye, rgb332_t c=0xF);

void rot_bar(uint8_t r)
{
    
    uint8_t  xc = FIRST_COL + (FRAMEBUFW >>1);
    uint8_t  yc = FIRST_ROW + (FRAMEBUFH >>1);
    uint8_t xs = xc - r;
    uint8_t ys = yc - r;
    uint8_t ye = yc + r; 

    clear(bg);
    if (rot_dir == 0)
    {
       drawdiag(xc, ys, 0, 1, 2*r, 0xFF);
    }
    if (rot_dir == 1)
       drawdiag(xs, ye, 1, -1, 2*r, 0xFF);
    if (rot_dir == 2)
        drawdiag(xs, yc, 1, 0, 2*r, 0xFF);
    if (rot_dir == 3 )
        drawdiag(xs, ys, 1, 1, 2*r, 0xFF);
    rot_dir = (rot_dir + 1) % 4;
}

#define ERR_XY_OUT_OF_RANGE -1

int setpixel(uint8_t x, uint8_t y, rgb332_t c)
{
  if (x > 0 && y > 0 && x < FRAMEBUFW && y < FRAMEBUFH); else return ERR_XY_OUT_OF_RANGE;
  framebuf[x][y] = c;
  return 0;
}

rgb332_t getpixel(uint8_t x, uint8_t y)
{
    if (x > 0 && y > 0 && x < FRAMEBUFW && y < FRAMEBUFH); else return 0;
    return framebuf[x][y];
}

void flushframe()
{
    SPI.beginTransaction(settingsTFT);
    TFT_CS_LOW();
        TFTwriteWindow(FIRST_COL, LAST_COL, FIRST_ROW, LAST_ROW);
        TFTwriteCommand(RAMWR); 
            for (uint8_t y = 0; y < FRAMEBUFH; ++y) 
            for (uint8_t x = 0; x < FRAMEBUFW; ++x)
            {
                rgb332_t c = getpixel(x, y);
                // send rgb332 converted to rgb565 by interspersing with zeroes 
                // RRRGGGBB -> R0R0R0G0 G0G0B0B0
                SPI.transfer(
                      c & _BV(7)      // bit 7
                    | c & _BV(6) >> 1 // bit 5
                    | c & _BV(5) >> 2 // bit 3
                    | c & _BV(4) >> 3 // bit 1
                );
                SPI.transfer(
                      c & _BV(3) << 4 // bit 7
                    | c & _BV(2) << 3 // bit 5
                    | c & _BV(1) << 2 // bit 3
                    | c & _BV(0) << 1 // bit 1
                );
            }
    TFT_CS_HIGH();
    SPI.endTransaction();
}


void clear(rgb332_t c)
{
  for (int x = 0; x < FRAMEBUFW; ++x)
    for (int y = 0; y < FRAMEBUFH; ++y)
      setpixel(x, y, c);
}
    
void drawrect(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye, rgb332_t c) {
    for (uint16_t i=xs; i<=xe; ++i) {
        for (uint16_t n=ys; n<=ye; ++n) {
             setpixel(i, n, c);
        }
    }
    flushframe();
}

void drawdiag(uint16_t sx, uint16_t sy, uint16_t dx, uint16_t dy, uint16_t l, rgb332_t c) {
    for (int i = 0; i < l; ++i)
    {
        setpixel(sx, sy, c);
        sx += dx; sy += dy;
    }
    flushframe();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 6.5

extern
unsigned char font[95][6];
#define ERR_INVALID_CHAR -2

int printChar(uint8_t x, uint8_t y, char c, rgb332_t fg, rgb332_t bg)
{
    if (c < 0x20 || c > 0x7e) return ERR_INVALID_CHAR;

    int result = 0;
    uint8_t *glyph = font[c - 0x20];
    for (uint8_t i = 0; i < 6; ++i)
    for (uint8_t j = 0; j < 8; ++j)
    {
        char cc = glyph[i] & _BV(j) ? fg : bg;
        result = result | setpixel(x+i, y+j, cc);
    }
    return result;
}

int printString(uint8_t x, uint8_t y, char const *str, rgb332_t fg, rgb332_t bg)
{
    while (*str)
    {
      int result = printChar(x, y, *str, fg, bg);
      if (result) return result;
      x+= 6;
      ++str;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
#define CHBUF_MAX 80
char chbuf[CHBUF_MAX] = "";
int chbuf_i = 0;


void loop() {
    int ch;
    if ((ch = Serial.read()) != -1)
    {
        chbuf[chbuf_i++] = ch;
        if (chbuf_i == CHBUF_MAX)
        {
            chbuf_i = 0;
            Serial.println("WARNING: Input buffer overflow.");
        }
        
        if (!isEOL(ch))
        {
            Serial.print((char) ch);
        }
        else
        {
            Serial.println("");
            
            void cmd(char const **);
            char const *line = chbuf;
            cmd(&line);
            Serial.println(line);
            
            chbuf_i = 0;
            chbuf[chbuf_i] = 0;
            Serial.print(" $ ");
        }
    }
}   

bool expect_str(char const **line, char const *str);
void skip_whitespace(char const **line);
bool expect_ulong_arg(char const **line, unsigned long *arg);

void cmd(char const **line)
{
    if (expect_str(line, "help"))
    {
        skip_whitespace(line);
        if (!isEOL((*line)[0]))
            ; // print help text anyways
        *line =
          "Commands:\r\n"
          "  * help\r\n"
          "  * clearDisplay\r\n"
          "  * runRotatingBarDemo\r\n"
          "  * runStudentIdDemo\r\n"
          "  * stopDemo";
    }
    else if (expect_str(line, "clearDisplay"))
    {
        if (!isEOL((*line)[0]))
            { *line = "Error: Expected end of line after 'clearDisplay'"; return; }
        clear(bg);
        flushframe();
    }
    else if (expect_str(line, "runRotatingBarDemo"))
    {
        if (!isEOL((*line)[0]))
            { *line = ("Error: Expected end of line after 'runRotatingBarDemo'"); return; }
        clear(bg);
        flushframe();
        rotatingbarenabled = true;
        Timer3.configure(10, on_timer);
        Timer3.start();
    }
    else if (expect_str(line, "runStudentIdDemo"))
    {
        if (!isEOL((*line)[0]))
            { *line = ("Error: Expected end of line after 'runStudentIdDemo'"); return; }
        clear(bg);
        flushframe();
        rotatingbarenabled = false;
        Timer3.configure_duration(5, on_timer);
        Timer3.start();
    }
    else if (expect_str(line, "stopDemo"))
    {
        if (!isEOL((*line)[0]))
            { *line = ("Error: Expected end of line after 'stopDemo'"); return; }
        Timer3.stop();
        clear(bg);
        flushframe();
    }
    else
    {
        *line = "help"; cmd(line);
    }
}


bool expect_str(char const **line, char const *str)
{
    if (strstr(*line, str) == *line)
    {
        *line = *line + strlen(str);
        return true;
    }
    else
    {
        return false;
    }
}

void skip_whitespace(char const **line)
{
    *line = *line + strspn(*line, " \x09\x0A\x0B\x0C\x0D");
}

bool expect_ulong_arg(char const **line, unsigned long *arg)
{
    char const *line_begin = *line;
    skip_whitespace(line);
    *arg = 0;
    if ((*line)[0] < '0' || (*line)[0] > '9')
      { *line = line_begin; return false; }
    while ((*line)[0] >= '0' && (*line)[0] <= '9')
    {
        *arg = 10 * *arg + (*line)[0] - '0';
        ++*line;
    }
    skip_whitespace(line);
    return true;
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t font[95][6] =
{ 
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // space
{ 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00 }, // !
{ 0x00, 0x07, 0x00, 0x07, 0x00, 0x00 }, // "
{ 0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00 }, // #
{ 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00 }, // $
{ 0x23, 0x13, 0x08, 0x64, 0x62, 0x00 }, // %
{ 0x36, 0x49, 0x55, 0x22, 0x50, 0x00 }, // &
{ 0x00, 0x00, 0x07, 0x00, 0x00, 0x00 }, // '
{ 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00 }, // (
{ 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00 }, // )
{ 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00 }, // *
{ 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 }, // +
{ 0x00, 0x50, 0x30, 0x00, 0x00, 0x00 }, // ,
{ 0x08, 0x08, 0x08, 0x08, 0x08, 0x00 }, // -
{ 0x00, 0x60, 0x60, 0x00, 0x00, 0x00 }, // .
{ 0x20, 0x10, 0x08, 0x04, 0x02, 0x00 }, // slash
{ 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 }, // 0
{ 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 }, // 1
{ 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 }, // 2
{ 0x21, 0x41, 0x45, 0x4B, 0x31, 0x00 }, // 3
{ 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 }, // 4
{ 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 }, // 5
{ 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00 }, // 6
{ 0x03, 0x71, 0x09, 0x05, 0x03, 0x00 }, // 7
{ 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 }, // 8
{ 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00 }, // 9
{ 0x00, 0x36, 0x36, 0x00, 0x00, 0x00 }, // :
{ 0x00, 0x56, 0x36, 0x00, 0x00, 0x00 }, // ;
{ 0x08, 0x14, 0x22, 0x41, 0x00, 0x00 }, // <
{ 0x14, 0x14, 0x14, 0x14, 0x14, 0x00 }, // =
{ 0x00, 0x41, 0x22, 0x14, 0x08, 0x00 }, // >
{ 0x02, 0x01, 0x51, 0x09, 0x06, 0x00 }, // ?
{ 0x32, 0x49, 0x79, 0x41, 0x3E, 0x00 }, // @
{ 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 }, // A
{ 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00 }, // B
{ 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 }, // C
{ 0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00 }, // D
{ 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00 }, // E
{ 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00 }, // F
{ 0x3E, 0x41, 0x41, 0x49, 0x7A, 0x00 }, // G
{ 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 }, // H
{ 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00 }, // I
{ 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00 }, // J
{ 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 }, // K
{ 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00 }, // L
{ 0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00 }, // M
{ 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00 }, // N
{ 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 }, // O
{ 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 }, // P
{ 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00 }, // Q
{ 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00 }, // R
{ 0x26, 0x49, 0x49, 0x49, 0x32, 0x00 }, // S
{ 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 }, // T
{ 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00 }, // U
{ 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00 }, // V
{ 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00 }, // W
{ 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 }, // X
{ 0x07, 0x08, 0x70, 0x08, 0x07, 0x00 }, // Y
{ 0x61, 0x51, 0x49, 0x45, 0x43, 0x00 }, // Z
{ 0x00, 0x7F, 0x41, 0x41, 0x00, 0x00 }, // [
{ 0x02, 0x04, 0x08, 0x10, 0x20, 0x00 }, // backslash
{ 0x00, 0x41, 0x41, 0x7F, 0x00, 0x00 }, // ]
{ 0x04, 0x02, 0x01, 0x02, 0x04, 0x00 }, // ^
{ 0x40, 0x40, 0x40, 0x40, 0x40, 0x00 }, // _
{ 0x00, 0x01, 0x02, 0x04, 0x00, 0x00 }, // `
{ 0x20, 0x54, 0x54, 0x54, 0x78, 0x00 }, // a
{ 0x7F, 0x48, 0x44, 0x44, 0x38, 0x00 }, // b
{ 0x38, 0x44, 0x44, 0x44, 0x20, 0x00 }, // c
{ 0x38, 0x44, 0x44, 0x48, 0x7F, 0x00 }, // d
{ 0x38, 0x54, 0x54, 0x54, 0x18, 0x00 }, // e
{ 0x08, 0x7E, 0x09, 0x01, 0x02, 0x00 }, // f
{ 0x08, 0x54, 0x54, 0x54, 0x3C, 0x00 }, // g
{ 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00 }, // h
{ 0x00, 0x48, 0x7D, 0x40, 0x00, 0x00 }, // i
{ 0x20, 0x40, 0x44, 0x3D, 0x00, 0x00 }, // j
{ 0x7F, 0x10, 0x28, 0x44, 0x00, 0x00 }, // k
{ 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00 }, // l
{ 0x7C, 0x04, 0x78, 0x04, 0x78, 0x00 }, // m
{ 0x7C, 0x08, 0x04, 0x04, 0x78, 0x00 }, // n
{ 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 }, // o
{ 0x7C, 0x14, 0x14, 0x14, 0x08, 0x00 }, // p
{ 0x08, 0x14, 0x14, 0x18, 0x7C, 0x00 }, // q
{ 0x7C, 0x08, 0x04, 0x04, 0x08, 0x00 }, // r
{ 0x48, 0x54, 0x54, 0x54, 0x20, 0x00 }, // s
{ 0x04, 0x3F, 0x44, 0x40, 0x20, 0x00 }, // t
{ 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00 }, // u
{ 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00 }, // v
{ 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00 }, // w
{ 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 }, // x
{ 0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00 }, // y
{ 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00 }, // z
{ 0x00, 0x08, 0x36, 0x41, 0x00, 0x00 }, // {
{ 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00 }, // |
{ 0x00, 0x41, 0x36, 0x08, 0x00, 0x00 }, // }
{ 0x10, 0x08, 0x08, 0x10, 0x08, 0x00 } // ~
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void initial_example_animation()
{
        //draw blue rectangle (50x50pixel) centered on Display
        uint8_t  xc = FIRST_COL + (FRAMEBUFW >>1);
        uint8_t  yc = FIRST_ROW + (FRAMEBUFH >>1);
        uint8_t xs = xc - 25;
        uint8_t xe = xc + 25;
        uint8_t ys = yc - 25;
        uint8_t ye = yc + 25;
    
        SPI.beginTransaction(settingsTFT);
        TFT_CS_LOW();
        TFTwriteWindow(xs, xe, ys, ye);
        TFTwriteCommand(RAMWR);  //assign blue-color to every element of writewindow
            for (uint16_t i=0; i<(xe+1-xs)*(ye+1-ys); i++) {
                SPI.transfer(0x001F>>8);
                SPI.transfer(0x001F);}
        TFT_CS_HIGH();
        SPI.endTransaction();
    
        delay(500);
    
        //draw centered red colored cross with one pixel space to border
        xs = FIRST_COL+1;
        xe = LAST_COL-1;
        ys = FIRST_ROW+1;
        ye = LAST_ROW-1;
        SPI.beginTransaction(settingsTFT);
        TFT_CS_LOW();
        //horizontal line
        TFTwriteWindow(xs, xe, yc, yc);
        TFTwriteCommand(RAMWR);  //assign background-color to every element of writewindow
            for (uint16_t i=0; i<(xe+1-xs); i++) {
                SPI.transfer(0xF800>>8);
                SPI.transfer(0xF800);}
        //vertical line
        TFTwriteWindow(xc, xc, ys, ye);
        TFTwriteCommand(RAMWR);  //assign background-color to every element of writewindow
            for (uint16_t i=0; i<(xe+1-xs); i++) {
                SPI.transfer(0xF800>>8);
                SPI.transfer(0xF800);}
        TFT_CS_HIGH();
        SPI.endTransaction();
}*/
