
// ##### Aufgabe 2.x #####
// analog input/output

#include "stdint.h"
#include "math.h"
#include "string.h"
#include "strutils.h"

#define CHBUF_MAX 80
char chbuf[CHBUF_MAX] = "";
int chbuf_i = 0;
float illuminance = 1.0f;
bool LEDon = false;

void setup()
{
    analogReference(EXTERNAL);
  
    #define buttonpin 3
    pinMode(buttonpin, INPUT);
    #define LEDpin 13
    pinMode(LEDpin, OUTPUT);
    #define potipin A9  //analog input poti
    pinMode(potipin, INPUT);
  
    Serial.begin(9600);
    Serial.print(" $ ");
}

void loop()
{
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
  
    long adc = analogRead(potipin);
    // f_PWM = 1 / 1ms = 1kHz
    if (LEDon)
      analogWrite(LEDpin, illuminance * 255);
    else
      analogWrite(LEDpin, adc * 256 / 1024);
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
          "  * LEDon\r\n"
          "  * LEDoff\r\n"
          "  * illum <percent>\r\n"
          "  * pins";
    }
    else if (expect_str(line, "LEDon"))
    {
        if (!isEOL((*line)[0]))
            { *line = "Error: Expected end of line after 'LEDon'"; return; }
        LEDon = true;
    }
    else if (expect_str(line, "LEDoff"))
    {
        if (!isEOL((*line)[0]))
            { *line = ("Error: Expected end of line after 'LEDoff'"); return; }
        LEDon = false;
    }
    else if (expect_str(line, "illum"))
    {
        unsigned long percent;
        if (!expect_ulong_arg(line, &percent) || percent > 100)
            { *line = ("Error: 'illum' expects exactly one positive whole-numbered argument not greater than 100."); return; }
        illuminance = percent / 100.f;
    }
    else if (expect_str(line, "pins"))
    {
        if (!isEOL((*line)[0]))
            { *line = ("Error: Expected end of line after 'pins'"); return; }
        int adc = analogRead(potipin);
        Serial.print("A9.ADC = ");
        Serial.print(adc);
        Serial.print("/1023 = ");
        Serial.print(float(adc)/1023 * 5);
        Serial.print("V/5V = ");
        Serial.print(float(adc)/1023 * 5000);
        Serial.print("mV/5000mV"); 
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
