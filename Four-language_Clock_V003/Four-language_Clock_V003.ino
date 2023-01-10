// =============================================================================================================================
/* 
This Arduino code controls the Nano Every, ATMEGA1284 chip and Arduino MKR1010 that controls the LED strips of the Word Clock
This source contains code for the following modules:  
- RTC DS3231 ZS-042 clock module
- KY-040 Keyes Rotary Encoder
- LDR light sensor 5528
- Bluetooth RF Transceiver Module HC05 and HM10 BLE
- DCF77 module DCF-2
- LCD
- WIFI on MKR 1010 to get NTP time

Load and install in IDE:
http://arduino.esp8266.com/stable/package_esp8266com_index.json  (ESP-12)
https://mcudude.github.io/MightyCore/package_MCUdude_MightyCore_index.json ATMEGA1260 and 644
https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json ATTINY
Arduino SAMD for MKR1010

The HC05 or HM-10 Bluetooth module is used to read and write information or instructions to the clock
The DCF77 module can be attached to adjust the time to the second with German longwave time signal received by the module.
The DCFtiny subroutine is  a non-interrupt driven DCF77 decoding routine. Using both together improves decoding efficiency

Arduino Uno, Nano with SK6812 LEDs: Program size must stay below approx 23572, 1129 bytes bytes with 144 LEDs in ATMEGA 32KB 
With a Arduino Nano Every with 64kb memory this is not a problem
************************************************************************************
To set the time with the rotary button:
One press on rotary: UUR is flashing -> turn to set hour
Second press on rotary: HT IS WAS flashes --> turn to set minute
the following presses are various display modes with the DIGITAL choice as second last 
Last press on rotary: many letters turns on --> turn to set intensity level of the LEDs
Press again of wait for one minute to return to normal operation
Displaymodes (Qn):
DEFAULTCOLOUR = 0; Yellow text.  HET Green to Red hour IS WAS Green to Red Minute  
HOURLYCOLOUR  = 1; Every hour other colour text. HET Green to Red hour IS WAS Green to Red Minute         
WHITECOLOR    = 2; All white text
OWNCOLOUR     = 3; All own colour
OWNHETISCLR   = 4; All own colour. HET Green to Red hour IS WAS Green to Red Minute
WHEELCOLOR    = 5; Every second another colour of rainbow
DIGITAL       = 6; Digital display
************************************************************************************
 Author .: Ed Nieuwenhuys 
 Program name; Four-language Clock_V0xx
 
 Changes.: V001 Adapted from Character_Colour_Clock_V060  Added || Dday>31  SIX ->SIXUK  
 Changes.: V002 Serialcheck in Demomode() and Selftest(). Changed own color entry to Pwwrrggbb instead of Prrggbb
                Counters for DCF Wrong times reported added. LedsOnOff added. sorage in Mem. EEPROM added
                Removed SetMinuteColour() and CheckColourStatus() and fused them in SetSecondColour(). 
                Optimized DCFtiny code TimeMinutesDiff <2 --> <10.Added EEPROM.put in setup.
                Added || Dday>31 in  updateDCFclock. sizeof(menu) / sizeof(menu[0]). Added PrintLine sub routine
Changes.: V003  Removed Changing color HET IS WAS per second
 */
// ===============================================================================================================================

//--------------------------------------------
// ARDUINO Definition of installed modules
//--------------------------------------------
//#define LED2812
#define LED6812          // choose between LED type RGB=LED2812 or RGBW=LED6812

#define ROTARYMOD
#define BLUETOOTHMOD     // Use  this define if Bluetooth needs other pins than pin 0 and pin 1
//#define HC12MOD            // Use HC12 time transreceiver Long Range Wireless Communication Module in Bluetooth slot
//#define DCF77MOD           // DCF77 receiver installed
#define MOD_DS3231       // The DS3231 module is installed, Otherwise the Arduino internal clock is used
//#define LCDMOD

char VERSION[] = "Four-language Clock_V003";
//--------------------------------------------
// ARDUINO Definition of installed language word clock
//--------------------------------------------
#define NL
#define UK
#define DE
#define FR        
//--------------------------------------------
// ARDUINO Includes defines and initialisations
//--------------------------------------------
                     #if defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)
#define ATmega644_1284   // Make this #ifdef easier to read
                     #endif
                     #if defined DCF77MOD
#define DCFMOD           // Use the Arduino DCF77 library with interrupts. Turning DCF77 off saves 4.4K bytes
#define DCFTINY          // Use the Tiny DCF algorithm in this program.    Turning DCFtiny off saves 4.0K bytes
                     #endif

#include <Wire.h>                // Arduino
#include "RTClib.h"              // https://github.com/adafruit/RTClib 
#include <TimeLib.h>             // For time management 
#include <avr/wdt.h>             // for reset function
#include <avr/pgmspace.h>        // Arduino
                     #if defined(ARDUINO_SAMD_MKRWIFI1010)
#include <FlashAsEEPROM.h>        // url=https://github.com/cmaglie/FlashStorage
#include <SPI.h>                  // for web server
#include <WiFiNINA.h>             // url=http://www.arduino.cc/en/Reference/WiFiNINA 
//#include "arduino_secrets.h"    // for web server
#define SECRET_SSID "PWDis_klok"
#define SECRET_PASS "klok"
                     #else if
#include <EEPROM.h>
                     #endif
                     #ifdef LCDMOD
#include <LCD.h>
#include <LiquidCrystal_I2C.h>     // https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
                     #endif  LCDMOD    
#include <Adafruit_NeoPixel.h>     // https://github.com/adafruit/Adafruit_NeoPixel 
                     #ifdef HC12MOD
#include <SoftwareSerial.h>        // For HC12 transceiver
                     #endif HC12MOD 
//                     #ifdef ATmega644_1284
                     #ifdef BLUETOOTHMOD
#include <SoftwareSerial.h>        // Arduino      // for Bluetooth communication
                     #endif BLUETOOTHMOD

//                     #endif ATmega644_1284
                     #ifdef ROTARYMOD
#include <Encoder.h>               // http://www.pjrc.com/teensy/td_libs_Encoder.html 
                     #ifdef ATmega644_1284
  #define CORE_NUM_INTERRUPT 3               // use other interupt voor these chipsetsa
  #define CORE_INT0_PIN   10
  #define CORE_INT1_PIN   11
  #define CORE_INT2_PIN   2
                     #endif
                     #endif ROTARYMOD
                     #ifdef DCFMOD
#include "DCF77.h"                     // http://playground.arduino.cc/Code/DCF77 
                     #endif DCFMOD
                     #ifdef NL
#define HET     ColorLeds("Het",      0,   2, LetterColor);   
#define IS      ColorLeds("is",       4,   5, LetterColor); 
#define WAS     ColorLeds("was",      8,  10, LetterColor); 
#define PRECIES ColorLeds("precies", 43,  49, LetterColor);
#define MTIEN   ColorLeds("tien",    38,  41, LetterColor); 
#define MVIJF   ColorLeds("vijf",    51,  54, LetterColor); 
#define KWART   ColorLeds("kwart",   56,  60, LetterColor);
#define VOOR    ColorLeds("voor",    96,  99, LetterColor);
#define OVER    ColorLeds("over",    90,  93, LetterColor);
#define HALF    ColorLeds("half",   100, 103, LetterColor);
#define MIDDER  ColorLeds("midder", 105, 110, LetterColor);
#define VIJF    ColorLeds("vijf",   144, 147, LetterColor);
#define TWEE    ColorLeds("twee",   138, 141, LetterColor);
#define EEN     ColorLeds("een",    150, 152, LetterColor);
#define VIER    ColorLeds("vier",   155, 158, LetterColor);
#define TIEN    ColorLeds("tien",   195, 198, LetterColor);
#define TWAALF  ColorLeds("twaalf", 188, 193, LetterColor);
#define DRIE    ColorLeds("drie",   200, 203, LetterColor);
#define NEGEN   ColorLeds("negen",  206, 210, LetterColor);
#define ACHT    ColorLeds("acht",   244, 247, LetterColor);
#define NACHT   ColorLeds("nacht",  244, 248, LetterColor);
#define ZES     ColorLeds("zes",    240, 242, LetterColor);
#define ZEVEN   ColorLeds("zeven",  251, 255, LetterColor);
#define ELF     ColorLeds("elf",    258, 260, LetterColor);
#define NOEN    ColorLeds("noen",   295, 298, LetterColor);
#define UUR     ColorLeds("uur",    290, 292, LetterColor);
#define EDSOFT  ColorLeds("EdSoft", 300, 311, LetterColor);
#define X_OFF   ColorLeds("",         0,   2, 0);
#define X_ON    ColorLeds("",         0,   2, LetterColor);
                     #endif NL
                     #ifdef UK
#define IT      ColorLeds("| It",   347, 348, UKLetterColor);   
#define ISUK    ColorLeds("is",     344, 345, UKLetterColor); 
#define WASUK   ColorLeds("was",    340, 342, UKLetterColor);
#define EXACTUK ColorLeds("exact",  351, 355, UKLetterColor);
#define HALFUK  ColorLeds("half",   357, 360, UKLetterColor); 
#define TWENTY  ColorLeds("twenty", 394, 399, UKLetterColor); 
#define MFIVE   ColorLeds("five",   388, 391, UKLetterColor);
#define QUARTER ColorLeds("quarter",400, 406, UKLetterColor);
#define MTEN    ColorLeds("ten",    409, 411, UKLetterColor);
#define PAST    ColorLeds("past",   446, 449, UKLetterColor);
#define TO      ColorLeds("to",     443, 444, UKLetterColor);
#define SIXUK   ColorLeds("six",    439, 441, UKLetterColor);
#define TWO     ColorLeds("two",    451, 453, UKLetterColor);
#define FIVE    ColorLeds("five",   457, 460, UKLetterColor);
#define TWELVE  ColorLeds("twelve", 494, 499, UKLetterColor);
#define TEN     ColorLeds("ten",    488, 490, UKLetterColor);
#define ELEVEN  ColorLeds("eleven", 500, 505, UKLetterColor);
#define FOUR    ColorLeds("four",   507, 510, UKLetterColor);
#define NINE    ColorLeds("nine",   545, 548, UKLetterColor);
#define THREE   ColorLeds("three",  538, 542, UKLetterColor);
#define EIGHT   ColorLeds("eight",  553, 557, UKLetterColor);
#define ONE     ColorLeds("one",    597, 599, UKLetterColor);
#define SEVEN   ColorLeds("seven",  589, 593, UKLetterColor);
#define OCLOCK  ColorLeds("O'clock",605, 610, UKLetterColor);
                      #endif UK
                      #ifdef DE
#define ES      ColorLeds("\nEs",   334, 335, DELetterColor);   
#define IST     ColorLeds("ist",    330, 332, DELetterColor); 
#define WAR     ColorLeds("war",    326, 328, DELetterColor); 
#define GENAU   ColorLeds("genau",  364, 368, DELetterColor);
#define MZEHN   ColorLeds("zehn",   370, 373, DELetterColor);
#define MFUNF   ColorLeds("funf",   383, 386, DELetterColor);
#define VIERTEL ColorLeds("viertel",375, 381, DELetterColor);
#define ZWANZIG ColorLeds("zwanzig",413, 419, DELetterColor);
#define KURZ    ColorLeds("kurz",   421, 424, DELetterColor);
#define VOR     ColorLeds("vor",    433, 435, DELetterColor);
#define NACH    ColorLeds("nach",   427, 430, DELetterColor);
#define HALB    ColorLeds("halb",   465, 468, DELetterColor);
#define FUNF    ColorLeds("funf",   471, 474, DELetterColor);
#define EINS    ColorLeds("eins",   483, 486, DELetterColor);
#define VIERDE  ColorLeds("vier",   478, 481, DELetterColor);
#define ZEHN    ColorLeds("zehn",   514, 517, DELetterColor);
#define ZWOLF   ColorLeds("zwolf",  520, 524, DELetterColor);
#define DREI    ColorLeds("drei",   532, 535, DELetterColor);
#define NEUN    ColorLeds("neun",   526, 529, DELetterColor);
#define ACHTDE  ColorLeds("acht",   565, 568, DELetterColor);
#define SECHS   ColorLeds("sechs",  570, 574, DELetterColor);
#define SIEBEN  ColorLeds("sieben", 580, 585, DELetterColor);
#define ZWEI    ColorLeds("zwei",   575, 578, DELetterColor);
#define ELFDE   ColorLeds("elf",    614, 616, DELetterColor);
#define UHR     ColorLeds("uhr",    621, 623, DELetterColor);
                      #endif DE
                      #ifdef FR
#define IL      ColorLeds("| Il",    13,  14, FRLetterColor);   
#define EST     ColorLeds("est",     16,  18, FRLetterColor); 
#define ETAIT   ColorLeds("etait",   20,  24, FRLetterColor);
#define EXACT   ColorLeds("exact",   31,  35, FRLetterColor);
#define SIX     ColorLeds("six",     27,  29, FRLetterColor); 
#define DEUX    ColorLeds("deux",    64,  67, FRLetterColor); 
#define TROIS   ColorLeds("trois",   69,  73, FRLetterColor);
#define ONZE    ColorLeds("onze",    83,  86, FRLetterColor);
#define QUATRE  ColorLeds("quatre",  75,  80, FRLetterColor);
#define MINUIT  ColorLeds("minuit", 113, 118, FRLetterColor);
#define DIX     ColorLeds("dix",    120, 122, FRLetterColor);
#define CINQ    ColorLeds("cinq",   133, 136, FRLetterColor);
#define NEUF    ColorLeds("neuf",   129, 132, FRLetterColor);
#define MIDI    ColorLeds("midi",   125, 128, FRLetterColor);
#define HUIT    ColorLeds("huit",   164, 167, FRLetterColor);
#define SEPT    ColorLeds("sept",   171, 174, FRLetterColor);
#define UNE     ColorLeds("une",    184, 186, FRLetterColor);
#define HEURE   ColorLeds("heure",  178, 182, FRLetterColor);
#define HEURES  ColorLeds("heures", 177, 182, FRLetterColor);
#define ET      ColorLeds("et",     213, 214, FRLetterColor);
#define MOINS   ColorLeds("moins",  216, 220, FRLetterColor);
#define LE      ColorLeds("le",     222, 223, FRLetterColor);
#define DEMI    ColorLeds("demie",  229, 233, FRLetterColor);
#define QUART   ColorLeds("quart",  263, 267, FRLetterColor);
#define MDIX    ColorLeds("dix",    270, 272, FRLetterColor);
#define VINGT   ColorLeds("vingt",  282, 286, FRLetterColor);
#define MCINQ   ColorLeds("cinq",   277 ,280, FRLetterColor);
#define DITLEHEURE DitLeHeure();
                      #endif FR
//--------------------------------------------
// PIN Assigments
//-------------------------------------------- 
             #if defined(__AVR_ATmega328P__) || defined(ARDUINO_SAMD_MKRWIFI1010) || defined (ARDUINO_AVR_NANO_EVERY)|| defined (ARDUINO_AVR_ATmega4809)
enum DigitalPinAssignments {      // Digital hardware constants ATMEGA 328 ----
 RX           = 0,                // Connects to Bluetooth TX
 TX           = 1,                // Connects to Bluetooth RX
 DCF_PIN      = 2,                // DCFPulse on interrupt  pin
 encoderPinA  = 3,                // right (labeled DT on decoder)on interrupt  pin
 clearButton  = 4,                // switch (labeled SW on decoder)
 LED_PIN      = 5,                // Pin to control colour SK6812/WS2812 LEDs
 BT_TX        = 6,                // Connects to Bluetooth RX
 BT_RX        = 7,                // Connects to Bluetooth TX
 HC_12TX      = 6,                // HC-12 TX Pin  // note RX and TX are reversed compared with a BT Module
 HC_12RX      = 7,                // HC-12 RX Pin 
 encoderPinB  = 8,                // left (labeled CLK on decoder)no interrupt pin  
 secondsPin   = 9,
 HeartbeatLED = 10,               // PIN10        = 10,                        // PIN 10 
 DCFgood      = 11,               // DCF-signal > 50    //PIN11        = 11,   // PIN 11
 PIN12        = 12,               // PIN 12             // led = 12,          // MKR1010
 DCF_LED_Pin  = 13,               // DCF signal
 };
 
enum AnaloguePinAssignments {     // Analogue hardware constants ----
 EmptyA0      = 0,                // Empty
 EmptyA1      = 1,                // Empty
 PhotoCellPin = 2,                // LDR pin
 EmptyA3      = 3,                // Empty
 SDA_pin      = 4,                // SDA pin
 SCL_pin      = 5,                // SCL pin
 EmptyA6     =  6,                // Empty
 EmptyA7     =  7};               // Empty
                   #endif

                             #ifdef ATmega644_1284  
//--------------------------------------------//--------------------------------------------
/*                     +---\/---+
           (D 0) PB0 1|        |40 PA0 (AI 0 / D24)
           (D 1) PB1 2|        |39 PA1 (AI 1 / D25)
      INT2 (D 2) PB2 3|        |38 PA2 (AI 2 / D26)
       PWM (D 3) PB3 4|        |37 PA3 (AI 3 / D27)
    PWM/SS (D 4) PB4 5|        |36 PA4 (AI 4 / D28)
      MOSI (D 5) PB5 6|        |35 PA5 (AI 5 / D29)
  PWM/MISO (D 6) PB6 7|        |34 PA6 (AI 6 / D30)
   PWM/SCK (D 7) PB7 8|        |33 PA7 (AI 7 / D31)
                 RST 9|        |32 AREF
                VCC 10|        |31 GND
                GND 11|        |30 AVCC
              XTAL2 12|        |29 PC7 (D 23)
              XTAL1 13|        |28 PC6 (D 22)
      RX0 (D 8) PD0 14|        |27 PC5 (D 21) TDI
      TX0 (D 9) PD1 15|        |26 PC4 (D 20) TDO
RX1/INT0 (D 10) PD2 16|        |25 PC3 (D 19) TMS
TX1/INT1 (D 11) PD3 17|        |24 PC2 (D 18) TCK
     PWM (D 12) PD4 18|        |23 PC1 (D 17) SDA
     PWM (D 13) PD5 19|        |22 PC0 (D 16) SCL
     PWM (D 14) PD6 20|        |21 PD7 (D 15) PWM
                      +--------+*/         
enum DigitalPinAssignments {      // Digital hardware constants ATMEGA 1284P ----
 BT_RX        = 0,                // Bluetooth RX Connects to Bluetooth TX
 BT_TX        = 1,                // Bluetooth TX Connects to Bluetooth RX
 DCF_PIN      = 2,                // DCFPulse on interrupt pin
 LED_PIN      = 3,                // Pin to control colour 2811/2812 leds PB3 digital
 PIN04        = 4,                // Pin 4         PB4 PWM
 secondsPin   = 5,                // Seconden
 PIN06        = 6,                // PIN 6         PB6 PWM  
 HeartbeatLED = 7,                // PIN 7         PB7 PWM 
 RX1          = 8,                // RX1           PD0 digital
 TX1          = 9,                // TX1           PD1 digital
 DCF_LED_Pin  = 10,               // LED10         PD2 digital
 encoderPinB  = 11,               // left (labeled CLK on decoder)no interrupt pin
 encoderPinA  = 12,               // right (labeled DT on decoder)on interrupt  pin
 clearButton  = 13,               // switch (labeled SW on decoder)  
 LED14        = 14,               //               PD6 digital
 LED15        = 15,               // LED15         PD7 PWM
 SCL_pin      = 16,               // SCL pin       PC0 interrupt
 SDA_pin      = 17,               // SDA pin       PC1 interrupt
 DCF_TX       = 19,               // TX2         4800,SERIAL_7E2);  //connect TX to D18 and RX to D19 PC2 digital
 DCF_RX       = 18,               // RX2 LED19     PC3 digital
 LEDDataPin   = 20,               // blauw HC595
 LEDStrobePin = 21,               // groen HC595
 LEDClockPin  = 22,               // geel  HC595
 PIN23        = 23 
 };
                                  
enum AnaloguePinAssignments {     // Analogue hardware constants ----
  EmptyA0     = 24,               // Empty
  EmptyA1     = 25,               // Empty
  PhotoCellPin= 26,               // LDR pin  AI 2
  EmptyA3     = 27,               // Empty
  EmptyA4     = 28,               // Empty
  EmptyA5     = 29,               // Empty
  EmptyA6     = 30};              // Empty
                     #endif ATmega644_1284

//--------------------------------------------
// COLOURS
//--------------------------------------------   

const byte DEFAULTCOLOUR = 0;
const byte HOURLYCOLOUR  = 1;          
const byte WHITECOLOR    = 2;
const byte OWNCOLOUR     = 3;
const byte OWNHETISCLR   = 4;
const byte WHEELCOLOR    = 5;
const byte DIGITAL       = 6;
byte ResetDisplayChoice  = DEFAULTCOLOUR; 
                         #ifdef LED2812
const uint32_t white     = 0x00FFFFFF; // white is R, G and B on WS2812                
                         #endif LED2812
                         #ifdef LED6812    
const uint32_t white     = 0xFF000000; // The SK6812 LED has a white LED that is pure white
                         #endif LED6812  
const uint32_t black  = 0x000000,  red   = 0xFF0000, orange     = 0xFF8000, darkorange = 0xFF8C00,  amber       = 0xFF7E00;
const uint32_t yellow = 0xFFFF00,  apple = 0x80FF00, brown      = 0x503000, cyberyellow = 0xFFD300, cadmiumyellow = 0xFFF600, chromeyellow = 0xFFA700;;
const uint32_t green  = 0x00FF00,  grass = 0x00FF80, sky        = 0x00FFFF, capri      = 0x00BFFF,  edamaranth  = 0xFF0050, amaranth      = 0xE52B50, brightgreen  = 0x66FF00;
const uint32_t marine = 0x0080FF,  blue  = 0x0000FF, pink       = 0xFF0080, purple     = 0xFF00FF,  darkviolet  = 0x800080, cerulean      = 0x007BA7, dodgerblue   = 0x0073FF;
const uint32_t edviolet = 0x7500BC,coquelicot = 0xFF3800, screamingreen= 0x70FF70, hotmagenta    = 0xFF00BF, frenchviolet = 0x8806CE            , greenblue    = 0x00F2A0;
//--------------------------------------------
// LED
//--------------------------------------------
const int  NUM_LEDS      = 625;         // How many leds in  strip?
const byte MATRIX_WIDTH  = 25;
const byte MATRIX_HEIGHT = 25;
const byte BRIGHTNESS    = 32;         // BRIGHTNESS 0 - 255
                         #ifdef LED6812    
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);  //NEO_RGBW
                         #endif LED6812  
                         #ifdef LED2812
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);  //NEO_RGB NEO_GRB
                         #endif LED2812
bool     LEDsAreOff            = false;     // If true LEDs are off except time display
bool     NoTextInColorLeds     = false;     // Flag to control printing of the text in function ColorLeds()
int      LEDNrCounter  = 0;  // For Matrix rain effect
byte     BrightnessCalcFromLDR = BRIGHTNESS;    // Initial brightness value 
int      Previous_LDR_read = 512;
uint32_t LetterColor   = white;
uint32_t UKLetterColor = green;   
uint32_t DELetterColor = red;
uint32_t FRLetterColor = yellow;
uint32_t DefaultColor  = white;      //0xFFFF00;    // Yellow
uint32_t UKDefaultColor= UKLetterColor;
uint32_t DEDefaultColor= DELetterColor;
uint32_t FRDefaultColor= FRLetterColor;
uint32_t OwnColour     = 0X002345DD;  // Blue
uint32_t WhiteColour   = white;
uint32_t WheelColor    = 0X000000FF;
uint32_t CrossColour   = yellow;
uint32_t HourColor[] ={  white,      darkviolet, cyberyellow, capri,         amber,         apple,
                         darkorange, cerulean,   edviolet,    cadmiumyellow, green,         edamaranth,
                         red,        yellow,     coquelicot,  pink,          apple,         hotmagenta,
                         green,      greenblue,  brightgreen, dodgerblue,    screamingreen, blue,
                         white,      darkviolet, cyberyellow};     

// Definition of the digits 0 - 9, 3 wide, 5 high. 
const byte PROGMEM Getal[10][3][5]  = { 
                     { {1, 1, 1, 1, 1}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1} },  //0
                     { {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 1} },  //1
                     { {1, 0, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 0, 1} },  //2
                     { {1, 0, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1} },  //3
                     { {1, 1, 1, 0, 0}, {0, 0, 1, 0, 0}, {1, 1, 1, 1, 1} },  //4
                     { {1, 1, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 0, 1, 1, 1} },  //5
//                   { {1, 1, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 0, 0, 1, 1} },  //5
                     { {1, 1, 1, 1, 1}, {0, 0, 1, 0, 1}, {0, 0, 1, 1, 1} },  //6
                     { {1, 1, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 1} },  //7
                     { {1, 1, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1} },  //8
                     { {1, 1, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1} }   //9
                     }; 
     
//--------------------------------------------
// KY-040 ROTARY
//-------------------------------------------- 
                          #ifdef ROTARYMOD                         
Encoder myEnc(encoderPinA, encoderPinB); // Use digital pin  for encoder
                          #endif ROTARYMOD      
long     Looptime          = 0;
byte     RotaryPress       = 0;          // Keeps track displaychoice and how often the rotary is pressed.
uint32_t RotaryPressTimer  = 0;
byte     NoofRotaryPressed = 0;

//--------------------------------------------
// LDR PHOTOCELL
//--------------------------------------------
const byte MAXBRIGHTNESS = 40;
const byte LOWBRIGHTNESS = 15;
int   OutPhotocell;                       // stores reading of photocell;
byte  TestLDR            = 0;             // If true LDR info is printed every second in serial monitor
int   MinPhotocell       = 999;           // stores minimum reading of photocell;
int   MaxPhotocell       = 1;             // stores maximum reading of photocell;

//--------------------------------------------
// CLOCK
//--------------------------------------------                                 
#define MAXTEXT 80
static  uint32_t msTick;                  // the number of millisecond ticks since we last incremented the second counter
int     count; 
byte    Isecond, Iminute, Ihour, Iday, Imonth, Iyear; 
byte    lastminute = 0, lasthour = 0, sayhour = 0;
bool    ChangeTime           = false;
bool    ChangeLightIntensity = false;
bool    Demo                 = false;
bool    Zelftest             = false;
bool    ZegUur               = true;      // Say or not Uur in NL clock

//--------------------------------------------
// DS3231 CLOCK MODULE
//--------------------------------------------
#define DS3231_I2C_ADDRESS          0x68
#define DS3231_TEMPERATURE_MSB      0x11
#define DS3231_TEMPERATURE_LSB      0x12
        #ifdef MOD_DS3231
RTC_DS3231 RTCklok;    //RTC_DS1307 RTC; 
        #else if
RTC_Millis RTCklok;   
        #endif 
DateTime Inow;

//--------------------------------------------
// BLUETOOTH
//--------------------------------------------                                     
                           #ifdef BLUETOOTHMOD               // Bluetooth ---------------------
//                         #ifdef ATmega644_1284 
SoftwareSerial Bluetooth(BT_RX, BT_TX);   // BT_RX <=> TXD on BT module, BT_TX <=> RXD on BT module
// SoftwareSerial mySerial(10, 11);       // RX, TX
//                         #endif
String  BluetoothString;
                           #endif BLUETOOTHMOD 
//--------------------------------------------
// HC-12 Long Range Wireless Communication Module
//--------------------------------------------
                     #ifdef HC12MOD
SoftwareSerial HC12(HC_12TX, HC_12RX);    // HC-12 TX Pin, HC-12 RX Pin
                     #endif HC12MOD                           
//--------------------------------------------
// DCF-2 DCF77 MODULE
//--------------------------------------------
byte   DCF_signal            = 40;        // is a proper time received?
bool   SeeDCFsignalInDisplay = false;     // if ON then the display line HET IS WAS will show the DCF77-signal received
bool   UseDCF                = true;      // Use the DCF-receiver or not
bool   PrintDebugInfo        = false;     // for showing debug info for DCFTINY
                    #ifdef DCFMOD 
                    #if defined(__AVR_ATmega328P__) 
#define DCF_INTERRUPT 0                   // DCF Interrupt number associated with DCF_PIN
                    #endif
                    #if defined ATmega644_1284 ||  defined ARDUINO_AVR_NANO_EVERY
#define DCF_INTERRUPT 2                   // DCF Interrupt number associated with DCF_PIN
                    #endif
const byte DCF_INVERTED = LOW;            // HIGH (HKW) or LOW (Reichelt). Some DCF modules invert the signal
DCF77 DCF = DCF77(DCF_PIN, DCF_INTERRUPT,DCF_INVERTED);
                    #endif DCFMOD                   
                    #ifdef DCFTINY
//--------------------------------------------
// DCF77 DCFtiny MODULE
//--------------------------------------------
bool     DCFEd              = false;      // Flag to detect if DCFtiny library received a good time
bool     DCFTh              = false;      // Flag to detect if DCF77 library received a good time
bool     DCFlocked          = false;      // Time received from DCF77.
byte     Dsecond, Dminute, Dhour;         // Variables of DCF decodings
byte     Dday, Dmonth, Dyear, Dwday;      // Variables of DCF decodings 
byte     StartOfEncodedTime = 0;          // Should be always one. Start if time signal
byte     BOnehourchange     = 0;          // Flag. Day light saving flag
byte     CSummertime        = 30;         // Counter. If >50 times CSummertime is received Summertime = true
byte     CWintertime        = 30;         // Counter. If >50 times CWintertime is received Wintertime = true
byte     COnehourchange     = 0;          // Counter. If >10 times COnehourchange is received Onehourchange = true
uint32_t DCFmsTick          = 0;          // the number of millisecond ticks since we last incremented the second counter
uint32_t SumSecondSignal    = 0;          // sum of digital signals ( 0 or 1) in 1 second
uint32_t SumSignalCounts    = 0;          // Noof of counted signals
uint32_t SignalFaults       = 0;          // Counter for SignalFaults per hour  
//byte     SignalBin[10];                 // For debugging to see in which time interval reception takes place
                    #endif DCFTINY                
                    #ifdef LCDMOD
//--------------------------------------------
// LCD Module
//--------------------------------------------
//LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7);// 0x27 is the I2C bus address for an unmodified backpack
LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7);
                    #endif 
//--------------------------------------------
// Webserver
//--------------------------------------------
                     #if defined(ARDUINO_SAMD_MKRWIFI1010)
char ssid[]  = SECRET_SSID;        // your network SSID (name)
char pass[]  = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                  // your network key Index number (needed only for WEP)
//int led      = LED_BUILTIN;
int status   = WL_IDLE_STATUS;
WiFiServer     server(80);
                     #endif

//----------------------------------------
// Common
//----------------------------------------
//byte hbval                = 128;    // Heartbeat initial intensity
//uint32_t last_time = 0;             // Heartbeat defines
//byte hbdelta              = 10;     // Determines how fast heartbeat is
char sptext[MAXTEXT+2];             // For common print use    
int  MilliSecondValue     = 1000;   // The duration of a second  minus 1 ms. Used in Demo mode
static uint32_t last_time = 0;      // Heartbeat defines
struct EEPROMstorage {              // Data storage in EEPROM to maintain them after power loss
  byte LightReducer;
  byte LowerBrightness;
  byte DisplayChoice;
  byte TurnOffLEDsAtHH;
  byte TurnOnLEDsAtHH;
  uint32_t OwnColour;               // Self defined colour for clock display
  uint32_t EdMinTijd;               // Counter of  valid time but invalid date
  uint32_t EdMin;                   // Counter of total valid date/times since start for DCFtiny 
  uint32_t ThMin;                   // Counter of totalvalid date/times since start for DCF77
  uint32_t EdTh;                    // Valid date/times since start counters for DCFtiny and DCF77
  uint32_t EdThMin;                 // Valid times per minute counters for DCFtiny and DCF77
  uint32_t ThWrong;                 // Different times reported DCFtiny and DCF77
  uint32_t EdWrong;                 // Different times reported DCFtiny and DCF77
  uint32_t ValidTimes;              // Counter of total recorded valid times
  uint32_t MinutesSinceStart;       // Counter of recorded valid and not valid times
} Mem = {0};
//--------------------------------------------
// Menu
//--------------------------------------------  

const char menu[][MAXTEXT] = {
 "4-language word clock",
 "Enter time as: hhmmss (132145)",
 "A Debug DCF-signal",
 "C (C00FFFF00) Cross colour (n=0-F)", 
 "D D15122017 is date 15 December 2017",
 "G DCF-signalinfo in display",
 "H Use DCF-receiver",
 "L (L5) Min lightintensity(0-255 bits)",
 "M (M90)Max lightintensity(1%-250%)",
 "N (N2208)Turn OFF LEDs between Nhhhh",
 "P (P00234F8A) own colour (n=0-F)",
 "Q Display Choice (Q0-6)",
 "  Q0= Default colour",
 "  Q1= Hourly colour",
 "  Q2= All white",
 "  Q3= Own colour+cross",
 "  Q4= Own colour",
 "  Q5= Wheel colour",
 "  Q6= Digital display",
 "I For this info",
 "R Reset to default settings",
 "S Self test",
 "W  Test LDR reading every second",
 "X (X50) Demo mode. ms delay (0-9999)",
 "Ed Nieuwenhuys December 2020" };
//  -------------------------------------   End Definitions  ---------------------------------------

//--------------------------------------------
// ARDUINO Loop
//--------------------------------------------
void loop(void)
{
 if (Demo)          Demomode();
 else if (Zelftest) { Selftest(); SerialCheck();}
 else
  { 
 //                             #ifdef ATmega644_1284
 // heartbeat();                // only heartbeat with ATMEGA1284
 //                             #endif
  EverySecondCheck();  
                              #if defined DCFMOD || defined DCFTINY
  if(UseDCF)  DCF77Check();
                              #endif DCFMOD || DCFTINY
 
                              #ifdef ROTARYMOD      
  RotaryEncoderCheck(); 
                              #endif ROTARYMOD 
  }
}  
//--------------------------------------------
// ARDUINO Setup
//--------------------------------------------
void setup()
{                                                            
 Serial.begin(SERIAL_SPEED);                                                // Setup the serial port to 9600 baud                                                        // Start the RTC-module  
 while (!Serial);                                                   // wait for serial port to connect. Needed for native USB port only
 Tekstprintln("\n*********\nSerial started"); 
 Wire.begin();                                                      // Start communication with I2C / TWI devices     
 pinMode(DCF_LED_Pin,  OUTPUT);                                     // For showing DCF-pulse with a LED
 pinMode(DCF_PIN,      INPUT_PULLUP);                               // Use Pin 2 for DCF input
 pinMode(DCFgood,      OUTPUT ); 
 pinMode(secondsPin,   OUTPUT );  
 pinMode(HeartbeatLED, OUTPUT );      
                          #ifdef MOD_DS3231
 RTCklok.begin();
 Tekstprintln("RTC DS3231 enabled");// start the RTC-module
                          # else if 
 RTCklok.begin(DateTime(F(__DATE__), F(__TIME__)));                 // if no RTC module is installed use the ATMEGAchip clock
 Tekstprintln("Internal clock enabled");   
                          #endif MOD_DS3231 
                          #ifdef BLUETOOTHMOD 
                          #if defined(ARDUINO_SAMD_MKRWIFI1010)
 Serial1.begin(9600);                                               // Bluetooth connected to Serial1
                          #else if
 Bluetooth.begin(9600);  
                          #endif ARDUINO_SAMD_MKRWIFI1010
 Tekstprintln("Bluetooth enabled");
                          #endif BLUETOOTHMOD
                          #ifdef ROTARYMOD   
 pinMode(encoderPinA,  INPUT_PULLUP);
 pinMode(encoderPinB,  INPUT_PULLUP);  
 pinMode(clearButton,  INPUT_PULLUP); 
 Tekstprintln("Rotary enabled"); 
 myEnc.write(0);                                                    // Clear Rotary encode buffer
                          #endif ROTARYMOD 
 strip.begin();
 strip.setBrightness(BRIGHTNESS); //BRIGHTNESS);
 ShowLeds();                                                        // Initialize all pixels to 'off' 
                          #ifdef LED6812    
 Tekstprintln("LEDs SK6812 enabled");
                          #endif LED6812  
                          #ifdef LED2812
 Tekstprintln("LEDs WS2812 enabled");
                          #endif LED2812 
                          #ifdef HC12MOD
 HC12.begin(9600);               // Serial port to HC12
 Tekstprintln("HC-12 time receiver enabled");
                          #endif HC12MOD                           
                          #ifdef DCFMOD
 DCF.Start();                                                       // Start the DCF-module
 Tekstprintln("DCF77 enabled");
                          #endif DCFMOD
                          #ifdef DCFTINY
 DCFmsTick = millis();                                              // Start of DCF 1 second loop
 DateTime LastTinyDCFtime = DateTime(__DATE__, __TIME__); 
 Tekstprintln("DCFtiny enabled");
                          #endif DCFTINY
                          #ifdef LCDMOD
 lcd.begin (16,2); // for 16 x 2 LCD module                         // Activate LCD module
 lcd.setBacklightPin(3,POSITIVE);
 lcd.setBacklight(HIGH);
 Tekstprintln("LCD enabled");
                          #endif LCDMOD
                          #if defined(ARDUINO_SAMD_MKRWIFI1010)
 Tekstprintln("MKR-1010 WIFI starting");
 Setup_MKRWIFI1010();                                               // Start the MKR1010
 Tekstprintln("MKR-1010 WIFI enabled");
                          #endif ARDUINO_SAMD_MKRWIFI1010
                          #ifdef ARDUINO_AVR_NANO_EVERY
 Tekstprintln("Compiled for ARDUINO AVR NANO EVERY");
                          #endif
                          #if defined(__AVR_ATmega328P__) 
 Tekstprintln("Compiled for ARDUINO AVR ATmega328P");
                          #endif
                          #ifdef ATmega644_1284
 Tekstprintln("Compiled for ARDUINO AVR ATmega644_1284");
                          #endif
 DateTime now = RTCklok.now();
 DateTime compiled = DateTime(__DATE__, __TIME__);
 if (now.unixtime() < compiled.unixtime()) 
  {
   Serial.println(F("RTC is older than compile time! Updating"));   // Following line sets the RTC to the date & time this sketch was compiled
   RTCklok.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  }
 EEPROM.get(0,Mem);                                                 // Get the data from EEPROM
 Mem.LightReducer    = constrain(Mem.LightReducer,1,250);           // 
 Mem.LowerBrightness = constrain(Mem.LowerBrightness,0,250);        // 
 Mem.DisplayChoice   = constrain(Mem.DisplayChoice,0,DIGITAL);      // 
 if ( Mem.OwnColour == 0 ) Mem.OwnColour = 0X002345DD;              // If memory is empty then store default value, blue  
 EEPROM.put(0,Mem);                                                 // update EEPROM if some data are out of the constrains                                                        
// Selftest();                                     // Play the selftest
 GetTijd(0);                                       // Get the time and store it in the proper variables
 SWversion();                                      // Display the version number of the software
 //Displaytime();
 SetColours();                                     // Set colours according chosen palette
 msTick = Looptime = millis();                     // Used in KY-040 rotary for debouncing and seconds check 
 } 

//--------------------------------------------
// ARDUINO Reset to default settings
//--------------------------------------------
void Reset(void)
{
 Mem.LightReducer     = MAXBRIGHTNESS;             // Factor to dim ledintensity with. Between 0.1 and 1 in steps of 0.05
 Mem.LowerBrightness  = LOWBRIGHTNESS;             // Lower limit of Brightness ( 0 - 255)
 Mem.DisplayChoice    = ResetDisplayChoice; 
 Mem.TurnOffLEDsAtHH  = Mem.TurnOnLEDsAtHH = 0;
 Mem.OwnColour        = 0X002345DD;                // Blue 
 Mem.EdMin            = 0;                         // Counter of total valid date/times since start for DCFtiny 
 Mem.ThMin            = 0;                         // Counter of totalvalid date/times since start for DCF77
 Mem.EdTh             = 0;                         // Valid date/times since start counters for DCFtiny and DCF77
 Mem.EdThMin          = 0;                         // Valid times per minute counters for DCFtiny and DCF77
 Mem.ThWrong          = 0;                         // Different times reported DCFtiny and DCF77
 Mem.EdWrong          = 0;                         // Different times reported DCFtiny and DCF77
 Mem.ValidTimes       = 0;                         // Counter of total recorded valid times
 Mem.MinutesSinceStart= 0;                         // Counter of recorded valid and not valid times
 MinPhotocell         = 999;                       // stores minimum reading of photocell;
 MaxPhotocell         = 1;                         // stores maximum reading of photocell;
 TestLDR              = 0;                         // if true LDR display is printed every second
 ChangeTime           = false;
 ChangeLightIntensity = false;
 Demo                 = false;
 Zelftest             = false;
 ZegUur               = true;                      // Say or not Uur in NL clock
 Selftest();                                       // Play the selftest
 GetTijd(0);                                       // Get the time and store it in the proper variables
 SWversion();                                      // Display the version number of the software
 Displaytime();
}
//--------------------------------------------
// ARDUINO Reset function
//--------------------------------------------
 void SoftwareReset( uint8_t prescaler) 
 {
  wdt_enable( prescaler);    // start watchdog with the provided prescaler
  while(1) {}
}
//--------------------------------------------
// ARDUINO Reset to default settings
//--------------------------------------------
void ResetArduino(void)
{
 SoftwareReset( WDTO_60MS); //WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS, WDTO_250MS, WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, and WDTO_8S
}
//--------------------------------------------
// CLOCK Version info
//--------------------------------------------
void SWversion(void) 
{ 
 PrintLine(40);
 for (int i = 0; i < sizeof(menu) / sizeof(menu[0]); Tekstprintln(menu[i++]));
 PrintLine(40);
 sprintf(sptext,"  Brightness Min: %3d bits  Max: %3d%%",Mem.LowerBrightness, Mem.LightReducer); Tekstprintln(sptext);
 sprintf(sptext,"    LDR read Min:%4d bits  Max:%3d bits",MinPhotocell, MaxPhotocell);           Tekstprintln(sptext); 
 sprintf(sptext,"LEDs off between: %0.2d - %0.2d",Mem.TurnOffLEDsAtHH, Mem.TurnOnLEDsAtHH);      Tekstprintln(sptext);
 sprintf(sptext,"  Display choice: %3d", Mem.DisplayChoice);                                     Tekstprintln(sptext);
 sprintf(sptext,"Software: %s",VERSION);                                                         Tekstprintln(sptext); 
 GetTijd(1);  
 PrintLine(40);
}
void PrintLine(byte Lengte)
{
 for (int n=0; n<Lengte; n++) {Serial.print(F("_"));} Serial.println(); 
}

//--------------------------------------------
// CLOCK Demo mode
//--------------------------------------------
void Demomode(void)
{
 if ( millis() - msTick == 10)   digitalWrite(secondsPin,LOW);      // Turn OFF the second on pin 13
 if ( millis() - msTick >= MilliSecondValue)                        // Flash the onboard Pin 13 Led so we know something is happening
 {    
  msTick = millis();                                                // second++; 
  digitalWrite(secondsPin,HIGH);                                    // turn ON the second on pin 13
  Isecond = 60-Iminute;
  if( ++Iminute >59) { Iminute = 0; Isecond = 0; Ihour++;}
  if( Ihour >12)                                                    // if hour is after 12 o'clock 
   {    
    Ihour = 9;                   
    if (++Mem.DisplayChoice > DIGITAL) Mem.DisplayChoice = 0;           // start with a following DISPLAYCHOICe AT Nine oclock
   }
  DimLeds(false);
  Displaytime();
  Tekstprintln("");
 }
}

//--------------------------------------------
// CLOCK common print routines
//--------------------------------------------
void Tekstprint(char tekst[])
{
 Serial.print(tekst);    
                          #ifdef BLUETOOTHMOD   
//                          #ifdef ATmega644_1284
 Bluetooth.print(tekst);  
//                          #endif
                          #if defined(ARDUINO_SAMD_MKRWIFI1010)
 Serial1.print(tekst);  
                          #endif
                          #endif BLUETOOTHMOD
}

void Tekstprintln(char tekst[])
{
 Serial.println(tekst);    
                          #ifdef BLUETOOTHMOD
//                          #ifdef ATmega644_1284
 Bluetooth.println(tekst);  
//                          #endif
                          #if defined(ARDUINO_SAMD_MKRWIFI1010)
 Serial1.println(tekst);  
                          #endif
                          #endif BLUETOOTHMOD
}
//--------------------------------------------
// CLOCK Update routine done every second
//--------------------------------------------
void EverySecondCheck(void)
{
  uint32_t msLeap = millis()- msTick; 
if (msLeap >9 && msLeap <12) digitalWrite(secondsPin,LOW); // Turn OFF the second on SecondsPin. Minimize DigitalWrites.
if (msLeap > 999)                                          // Every second enter the loop
  {
    GetTijd(0);                                            // Update Isecond, Iminute, Ihour, Iday, Imonth, Iyear
    msTick = millis();
    digitalWrite(secondsPin,HIGH);                         // turn ON the second on pin 
                             #ifdef LCDMOD
   Print_tijd_LCD();
                             #endif LCDMOD    
//   if(Isecond % 30 == 0) DimLeds(true);                  // Text LED intensity control + seconds tick print every 30 seconds  
   DimLeds(TestLDR);    
                              #if defined(ARDUINO_SAMD_MKRWIFI1010)
   WIFIServercheck(); 
                              #endif
   SerialCheck();
                              #ifdef BLUETOOTHMOD   
   BluetoothCheck(); 
                              #endif BLUETOOTHMOD                          
                              #ifdef HC12MOD
   HC12Check();
                              #endif HC12MOD
  }
  if (Iminute != lastminute)   EveryMinuteUpdate();        // Enter the every minute routine after one minute
 }
//--------------------------------------------
// CLOCK Update routine done every minute
//--------------------------------------------
  void EveryMinuteUpdate(void)
 {
  static byte lastday = 0;
  lastminute = Iminute;  
  GetTijd(0);
  Displaytime();
  Print_RTC_tijd();
  SetColours();                                            // Set the colour according palette choice 
                    #ifdef DCFTINY
  if(Mem.MinutesSinceStart > 0X0FFFFFF0)                   // Reset counters before they overflow
    { Mem.EdMinTijd = Mem.EdMin = Mem.ThMin = Mem.EdThMin = Mem.ValidTimes = Mem.MinutesSinceStart = 0; }
  if(BOnehourchange >40)                                   // If this signal is received 40 times this day it must be true
    {                                                      // The BOnehourchange counter is resetted every day to zero 
     DCFlocked = false;                                    // RTC time can be updated from DCF time;
     sprintf(sptext,"One hour change commencing. BOnehourchange count: %0.2d ", BOnehourchange);
     Tekstprintln(sptext);  
    }
                    #endif DCFTINY
  if(Ihour != lasthour) 
    {
      lasthour = Ihour;
      if(Ihour == Mem.TurnOffLEDsAtHH) {LEDsAreOff = true;  LedsOff(); ShowLeds();}         // is it time to turn off the LEDs?
      if(Ihour == Mem.TurnOnLEDsAtHH)  LEDsAreOff = false;                                  // or on?
                    #ifdef DCFTINY
      SignalFaults = 0;                                     // Reset every hour. 
                    #endif DCFTINY
     }
   if(Iday != lastday) 
    {
      lastday = Iday; 
      EEPROM.put(0,Mem);                                   // This function uses EEPROM.update() to perform the write, so does not rewrites the value if it didn't change
                    #ifdef DCFTINY
      DCFlocked = false;                                   // RTC time can be updated from DCF time  Tekstprintln("DCFlock is unlocked ");
      BOnehourchange = 0;                                  // Reset every day.  
      MinPhotocell       = 999;                            // stores minimum reading of photocell;
      MaxPhotocell       = 1;                              // stores maximum reading of photocell;
                    #endif DCFTINY
     }
 }
//--------------------------------------------
// CLOCK check for serial input
//--------------------------------------------
void SerialCheck(void)
{
 String  SerialString; 
 while (Serial.available())
  { 
   char c = Serial.read();
   if (c>31 && c<128) SerialString += c;                   // allow input from Space - Del
   else c = 0;                                             // delete a CR
  }
 if (SerialString.length()>0) 
         ReworkInputString(SerialString);                  // Rework ReworkInputString();
 SerialString = "";

}
                           #ifdef HC12MOD
//--------------------------------------------
// CLOCK check for HC-12 input
//--------------------------------------------                            
void HC12Check(void)
{
 static bool DataInBuffer = false;
 String      HC12String; 
 static long HC12StartTime = millis(); 
 char        c;
 HC12String.reserve(64);
 HC12String ="";
 HC12.listen(); //When using two software serial ports, you have to switch ports by listen()ing on each one in turn.
 while (HC12.available()>0)                                // If HC-12 has data
   {       
    c = HC12.read();     
    if (c>31 && c<128)  HC12String += c;                   // Allow only input from Space - Del

    delay(3);
   }
 HC12String += "\n";
 DataInBuffer = false;
 if (HC12String.length()>0) 
   {
    Serial.print("Received HC-12: "); Serial.println(HC12String);
    ReworkInputString(HC12String);
    DataInBuffer = false;  
    }                  
}                         
                           #endif HC12MOD
                           #ifdef BLUETOOTHMOD
//--------------------------------------------
// CLOCK check for Bluetooth input
//--------------------------------------------                           
void BluetoothCheck(void)
{                          
                           #if defined(ARDUINO_SAMD_MKRWIFI1010) 
 while (Serial1.available()) 
  {
   char c = Serial1.read();
   Serial.print(c);
   if (c>31 && c<128) BluetoothString += c;
   else c = 0;     // delete a CR
  }
                           #else if
 Bluetooth.listen();                                       //  When using two software serial ports, you have to switch ports by listen()ing on each one in turn.
 while (Bluetooth.available()) 
  {
   char c = Bluetooth.read();
   if (c>31 && c<128) BluetoothString += c;
   else c = 0;     // delete a CR
  }
                           #endif   
 if (BluetoothString.length()>0)  
    ReworkInputString(BluetoothString);                   // Rework ReworkInputString();
 BluetoothString = "";
}
                           #endif BLUETOOTHMOD                          
                           #if defined DCFTINY || defined DCFMOD
//--------------------------------------------
// CLOCK check for DCF input
// This subroutine assumes that both DCF-receivers are installed with the #define 
// At the end of this routine +/- line 1069 at comment //<-- ******** here
// if (strcmp(sptext,sptextDCF77) || ( MinutesSinceStart <60 ) || !DCFlocked)
// If the time stamp of both methods are identical the time is updated in the RTC
//--------------------------------------------
void DCF77Check(void)
{
 digitalWrite(DCF_LED_Pin, !digitalRead(DCF_PIN));         // Write received signal to the DCF LED 
 // digitalWrite(DCF_LED_Pin, !digitalRead(DCF_PIN)*BrightnessCalcFromLDR/5);         // Write received signal to the DCF LED 
                              #ifdef DCFMOD
 tmElements_t tm;                                          // On compile error see text above
 time_t DCFtime = DCF.getTime();                           // Check if new DCF77 time is available
 if (DCFtime!=0)
  {
   breakTime(DCFtime, tm);                                 // Store the DCFtime in struct tm
   setTime(DCFtime);                                       // Needed for DCF77 lib
   sprintf(sptext,"  DCF77 Lib OK --> %0.2d:%0.2d %0.2d-%0.2d-%0.4d ",tm.Hour, tm.Minute, tm.Day, tm.Month, tm.Year+1970); 
   Tekstprintln(sptext); 
   DateTime now = RTCklok.now();                           // Needed to use unixtime.
   DateTime DCFtijd (1970+tm.Year, tm.Month, tm.Day, tm.Hour, tm.Minute, 0);   // Convert the time in the time_t DCFtime to Datetime so we can use .unixtime 
   uint32_t TimeDiff = (abs(now.unixtime() - DCFtijd.unixtime()));
   if(TimeDiff > 2 )                                       // > in Seconds. If difference between DCF and RTC time update the RTC
      {
       sprintf(sptext,"%lds difference Clock:%0.2d:%0.2d:%0.2d DCF77:%0.2d:%0.2d:%0.2d ",
                        TimeDiff,now.hour(), now.minute(),now.second(), tm.Hour,tm.Minute, tm.Second); 
       Tekstprintln(sptext);       
                             #ifndef DCFTINY
       RTCklok.adjust(DCFtime);                             // Update time here if DCFTINY is not used otherwise both receiver must received the same date-time
       Tekstprint(" ----> DCF77 Time is updated ");               // after one hour only if DCFlocked = true
       Tekstprintln(sptext); 
                             #endif DCFTINY
     } 
                             #ifdef DCFTINY
   Mem.ThMin++;
   DCFTh = true;                                           // Used to check if DCF77 and DCFtiny receive both a good time    
                             #endif DCFTINY
  }
                             #endif DCFMOD 
                             #ifndef DCFMOD 
  DCFTh = true; 
                             #endif DCFMOD    
                             #ifdef DCFTINY 
 SumSecondSignal += (!digitalRead(DCF_PIN));               // invert 0 to 1 and 1 to 0
 SumSignalCounts++; 
 if((millis() - DCFmsTick) >995) //= 1000)                 // Compute every second the received DCF-bit to a time 
   { 
    while (!digitalRead(DCF_PIN))                          // Avoid splitting a positive signal 
      {
       SumSignalCounts++;        
       SumSecondSignal++;
      }
    DCFmsTick = millis();                                  // Set one second counter
    SumSignalCounts *= 1.10;                               // correction to correct pulses to an average of 100 and 200 msecs
    if(byte OKstatus = UpdateDCFclock())                   // If after 60 sec a valid time is calculated, sent it to the HC-12 module
    if(OKstatus == 2 || OKstatus == 1)
      { 
       GetTijd(0);                                         // Store RTC time in Iday etc.
       if(OKstatus == 2)                                   // If time flag was OK and date flag was NOK
         {
          sprintf(sptext,"       TIME OK --> %0.2d:%0.2d  ",Dhour, Dminute); 
          Tekstprintln(sptext); 
          Dday   = Iday;
          Dmonth = Imonth;
          Dyear  = Iyear-2000; 
          Mem.EdMinTijd++;                                 // Counter for TIME OK
          return;                                          // if return is removed time will be updated with the new time
          OKstatus = 1;                                    // and the date from the RTC if it differs moren van 2 seconds 
         }
       sprintf(sptext,"TIME & DATE OK --> %0.2d:%0.2d %0.2d-%0.2d-%0.4d/%0.1d ",Dhour, Dminute, Dday, Dmonth, 2000+Dyear, Dwday );
       Tekstprintln(sptext);
                              #ifdef DCFMOD  
       char sptextDCF77[30];                                                    // Check if both DCF methods gives identical times when reporting a valid time
       char sptextRTC[30];
       sprintf(sptextDCF77,"%0.2d:%0.2d %0.2d-%0.2d-%0.4d",
                            tm.Hour, tm.Minute, tm.Day, tm.Month, tm.Year+1970);   
       sprintf(sptext,     "%0.2d:%0.2d %0.2d-%0.2d-%0.4d",
                           Dhour, Dminute, Dday, Dmonth, 2000+Dyear );   
       sprintf(sptextRTC,  "%0.2d:%0.2d %0.2d-%0.2d-%0.4d",
                           Ihour,Iminute,Iday, Imonth, Iyear);            
       if(DCFTh && strcmp(sptext,sptextDCF77)!=0)                               // If DCF77 received a time and if reported date/times are different
         {
          char fftext[90];
          sprintf(fftext,"Differences! -- \nRTC:%s\n Ed:%s\n Th:%s", 
          sptextRTC, sptext, sptextDCF77); 
          Tekstprintln(fftext);
          if(strcmp(sptextRTC,sptextDCF77)!=0)  { Mem.ThWrong++; Mem.ThMin--; DCFTh = false; }
          if(strcmp(sptextRTC,sptext)!=0)       { Mem.EdWrong++; return; }      // Exit the function
         }
                              #endif DCFMOD             
       DateTime now = RTCklok.now();                                            // Copy the time from the RTC in now 
       DateTime DCFtijd (Dyear, Dmonth, Dday, Dhour, Dminute, 0);  // and the DCFtiny time in DCFtijd
       uint32_t TimeDiff = (abs(now.unixtime() - DCFtijd.unixtime()));
       if(TimeDiff > 2)                                                         // > in Seconds. If difference between DCF and RTC time update the RTC
         {
          sprintf(sptext,"%lds difference! Clock:%0.2d:%0.2d:%0.2d DCFtiny:%0.2d:%0.2d:%0.2d ",
                            TimeDiff,Inow.hour(), Inow.minute(),Inow.second(),Dhour,Dminute,1); 
          Tekstprintln(sptext);
          char sptextDCF77[40];                                                 // Store timestamp here and compare it with the DCFtiny time stamp
                              #ifdef DCFMOD  
          sprintf(sptextDCF77,"%0.2d:%0.2d %0.2d-%0.2d-%0.4d",tm.Hour, tm.Minute, tm.Day, tm.Month, tm.Year+1970);
          Tekstprint("DCFLib ");     Tekstprint(sptextDCF77);
                              #else if
          sprintf(sptextDCF77,"%0.2d:%0.2d %0.2d-%0.2d-%0.4d",Dhour, Dminute, Dday, Dmonth, 2000+Dyear, Dwday );
                              #endif DCFMOD            
          sprintf(sptext,"%0.2d:%0.2d %0.2d-%0.2d-%0.4d",Dhour, Dminute, Dday, Dmonth, 2000+Dyear, Dwday );
          Tekstprint(" DCFtiny ");   Tekstprintln(sptext);           
          if(strcmp(sptext,sptextDCF77)==0 || ( Mem.MinutesSinceStart <60 ))    // Time+date is updated 60 minutes after start or DCFlocked==false   
            {                                                                   // or after 60 sec if DCF77 and DCFtiny received the same time-date   
             RTCklok.adjust(DateTime(Dyear, Dmonth, Dday, Dhour, Dminute, 0));
             if (Mem.MinutesSinceStart > 60) DCFlocked = true;                  // Every DCF time with OK status 1 or 2 are stored in the first 60 miutes
                                                                                // Every hour the DCFlocked is set to false
             Tekstprint(" ----> DCFTINY Time is updated ");  Tekstprintln(sptext);      // after one hour only if DCFlocked = true
            }
         } 
       Mem.EdMin++;                                                             // Count the valid times
       DCFEd = true;                                                            // Used to check if DCF77 and DCFtiny receive both a good time 
     }             
   }   // end msec >995
                             #endif DCFTINY                                         
} 

                             #ifdef DCFTINY
//--------------------------------------------
// DCFtiny Make the time from the received DCF-bits
//--------------------------------------------
byte UpdateDCFclock(void)
{
        byte     TimeOK     , Receivebit     , Receivebitinfo;                 // Return flag is proper time is computed and received bit value
        byte     BMinutemark, BReserveantenna; 
        byte     BSummertime, BWintertime    , BLeapsecond;
        bool     TimeSignaldoubtfull = false;                                  // If a bit length signal is not between 5% and 30%
 static byte     Bitpos,  Paritybit;                                           // Postion of the received second signal in the outer ring  
 static byte     MinOK=2, HourOK=2, YearOK=2;                                  // 2 means not set. static because the content must be remembered
// static int      TimeMinutes;  
// static int      LastTimeMinutes;
// static int      TimeMinutesDiff, DateDaysDiff;
// static uint32_t DateDays ,LastDateDays;
 
 if (Bitpos >= 60) 
     {
      Bitpos = 0;
      Mem.MinutesSinceStart++;   
      if (DCFEd && DCFTh) { Mem.EdThMin++; DCFlocked = true; }                 // Count the times both DCFalgorithm received a good time. UnLock DCFtime
      if (DCFEd || DCFTh) { DCF_signal++;  Mem.ValidTimes++; }
      else  DCF_signal--;                                            
      DCF_signal = constrain( DCF_signal,1,99);
      DCFEd = false;                                                           // Turn off good DCF-times received
      DCFTh = false;                                              
      } 
 int msec  = (int)(1000 * SumSecondSignal / SumSignalCounts);                  // This are roughly the milliseconds times 10 of the signal length
 switch(msec)
  {
   case   0 ...  10: if (SumSignalCounts > 50 && Bitpos > 5)                   // Check if there were enough signals and there were several positive signals 
                       {Bitpos = 59; Receivebitinfo = 2; Receivebit = 0; }     // If enough signals and one second no signal found. This is position zero
                              else { Receivebitinfo = 9; Receivebit = 0; }     // If not it is a bad signal probably a zero bit
                                                                        break; // If signal is less than 0.05 sec long than it is the start of minute signal
   case  11 ...  50: Receivebitinfo = 8; Receivebit = 0;                break; // In all other cases it is an error
   case  51 ... 160: Receivebitinfo = 0; Receivebit = 0;                break; // If signal is 0.1 sec long than it is a 0 
   case 161 ... 320: Receivebitinfo = 1; Receivebit = 1;                break; // If signal is 0.2 sec long than it is a 1 
   default:          Receivebitinfo = 9; Receivebit = 1;                break; // In all other cases it is an error probably a one bit
  } 
 if(Receivebitinfo > 2)  SignalFaults++;                                       // Count the number of faults per hour
 TimeOK = 0;                                                                   // Set Time return code to false  
 switch (Bitpos)                                                               // Decode the 59 bits to a time and date.
  {                                                                            // It is not binary but "Binary-coded decimal". Blocks are checked with an even parity bit. 
    case 0: BMinutemark = Receivebit;                                          // Blocks are checked with an even parity bit. 

                                                                        break; 
   case   1: TimeSignaldoubtfull = 0;                                          // Start a fresh minute
             sprintf(sptext,"--> EdT:%ld Ed:%ld Th:%ld EdW:%ld ThW:%ld Both:%ld Valid:%ld Min:%ld OK:%d Lock:%s",    
             Mem.EdMinTijd, Mem.EdMin, Mem.ThMin, Mem.EdWrong, Mem.ThWrong, Mem.EdThMin, Mem.ValidTimes, Mem.MinutesSinceStart,TimeOK,DCFlocked?"Y":"N");
             Tekstprintln(sptext);              
//             LastTimeMinutes = TimeMinutes;  
//             LastDateDays    = DateDays;
             Dminute = Dhour = Dday = Dwday = Dmonth = Dyear = 0;              // Reset the variables
             MinOK = HourOK  = YearOK = 2;                                      // Gives the dot (.) at MHY or mhy
   case 2 ... 14:                                                       break; // reserved for wheather info we will not use
   case  15: BReserveantenna = Receivebit;                              break; // 1 if reserve antenna is in use
   case  16: BOnehourchange  = Receivebit;
             if(Receivebit) COnehourchange++;
             else           COnehourchange--;
             COnehourchange = constrain(COnehourchange,0,15);
             if(COnehourchange>10) DCFlocked = false;                          // Unlock if one hour change comes up 
                                                                        break; // Has value of 1 one hour before change summer/winter time
   case  17: BSummertime = Receivebit;
             if(Receivebit) CSummertime++; 
             else           CSummertime--;
             CSummertime = constrain(CSummertime,5,60);                
                                                                        break;                               // 1 if summer time 
   case  18: BWintertime = Receivebit; 
             if(Receivebit) CWintertime++;
             else           CWintertime--;
             CWintertime = constrain(CWintertime,5,60);
                                                                        break; // 1 if winter time
   case  19: BLeapsecond = Receivebit;                                  break; // Announcement of leap second in time setting is coming up
   case  20: StartOfEncodedTime=Receivebit; Paritybit = 0;              break; // Bit must always be 1 
   case  21: if(Receivebit) {Dminute  = 1;  Paritybit = 1 - Paritybit;} break;
   case  22: if(Receivebit) {Dminute += 2 ; Paritybit = 1 - Paritybit;} break;
   case  23: if(Receivebit) {Dminute += 4 ; Paritybit = 1 - Paritybit;} break;
   case  24: if(Receivebit) {Dminute += 8 ; Paritybit = 1 - Paritybit;} break;
   case  25: if(Receivebit) {Dminute += 10; Paritybit = 1 - Paritybit;} break;
   case  26: if(Receivebit) {Dminute += 20; Paritybit = 1 - Paritybit;} break;
   case  27: if(Receivebit) {Dminute += 40; Paritybit = 1 - Paritybit;} break;
   case  28: MinOK = (Receivebit==Paritybit);                                  // The minute parity is OK or NOK
             Paritybit = 0;                                             break;    
   case  29: if(Receivebit) {Dhour   =  1; Paritybit = 1 - Paritybit;}  break;
   case  30: if(Receivebit) {Dhour  +=  2; Paritybit = 1 - Paritybit;}  break;
   case  31: if(Receivebit) {Dhour  +=  4; Paritybit = 1 - Paritybit;}  break;
   case  32: if(Receivebit) {Dhour  +=  8; Paritybit = 1 - Paritybit;}  break;
   case  33: if(Receivebit) {Dhour  += 10; Paritybit = 1 - Paritybit;}  break;
   case  34: if(Receivebit) {Dhour  += 20; Paritybit = 1 - Paritybit;}  break;
   case  35: HourOK = (Receivebit==Paritybit);                                 // The hour parity is OK or NOK         
             Paritybit = 0;                                             break;  
   case  36: if(Receivebit) {Dday    =  1; Paritybit = 1 - Paritybit;}  break;
   case  37: if(Receivebit) {Dday   +=  2; Paritybit = 1 - Paritybit;}  break;
   case  38: if(Receivebit) {Dday   +=  4; Paritybit = 1 - Paritybit;}  break;
   case  39: if(Receivebit) {Dday   +=  8; Paritybit = 1 - Paritybit;}  break;
   case  40: if(Receivebit) {Dday   += 10; Paritybit = 1 - Paritybit;}  break;
   case  41: if(Receivebit) {Dday   += 20; Paritybit = 1 - Paritybit;}  break;
   case  42: if(Receivebit) {Dwday   =  1; Paritybit = 1 - Paritybit;}  break;
   case  43: if(Receivebit) {Dwday  +=  2; Paritybit = 1 - Paritybit;}  break;
   case  44: if(Receivebit) {Dwday  +=  4; Paritybit = 1 - Paritybit;}  break;
   case  45: if(Receivebit) {Dmonth  =  1; Paritybit = 1 - Paritybit;}  break;
   case  46: if(Receivebit) {Dmonth +=  2; Paritybit = 1 - Paritybit;}  break;
   case  47: if(Receivebit) {Dmonth +=  4; Paritybit = 1 - Paritybit;}  break;
   case  48: if(Receivebit) {Dmonth +=  8; Paritybit = 1 - Paritybit;}  break;
   case  49: if(Receivebit) {Dmonth += 10; Paritybit = 1 - Paritybit;}  break;
   case  50: if(Receivebit) {Dyear   =  1; Paritybit = 1 - Paritybit;}  break;
   case  51: if(Receivebit) {Dyear  +=  2; Paritybit = 1 - Paritybit;}  break;
   case  52: if(Receivebit) {Dyear  +=  4; Paritybit = 1 - Paritybit;}  break;
   case  53: if(Receivebit) {Dyear  +=  8; Paritybit = 1 - Paritybit;}  break;
   case  54: if(Receivebit) {Dyear  += 10; Paritybit = 1 - Paritybit;}  break;
   case  55: if(Receivebit) {Dyear  += 20; Paritybit = 1 - Paritybit;}  break;
   case  56: if(Receivebit) {Dyear  += 40; Paritybit = 1 - Paritybit;}  break;
   case  57: if(Receivebit) {Dyear  += 80; Paritybit = 1 - Paritybit;}  break;
   case  58: YearOK = (Receivebit==Paritybit);                                 // The year month day parity is OK or NOK   
             Paritybit = 0;                                             break;  
   case  59: if( (MinOK && HourOK && YearOK) ) //&& ( TimeMinutesDiff < 10) && (DateDaysDiff < 2 ) )     
               {                                                   TimeOK = 1; }          // All OK 
             if( (MinOK && HourOK && !YearOK) ) //&& (TimeMinutesDiff<2)) 
               {                                                   TimeOK = 2; }          // If a date difference time remains usable                                                                   
             if(TimeSignaldoubtfull)                               TimeOK = 5;            // If a signal lenght was too short or long this last minute time is not OK
             if(Dmonth==0 || Dmonth>12 || Dday>31|| Dhour>23 || Dminute>59 ) TimeOK = 0;  // If time is definitely wrong return Time is not OK
//                      sprintf(sptext,"Min:%s Hour:%s Year:%s, Timediff:%d Datediff:%d", 
//                        MinOK==1?"OK":"NOK",HourOK==1?"OK":"NOK",YearOK==1?"OK":"NOK",TimeMinutesDiff,DateDaysDiff);  Tekstprintln(sptext); 
             //Serial.println("silence");                               
                                                                        break;
    default:  Serial.println(F("Default in BitPos loop. Impossible"));  break;
   }
  if((Bitpos>19 && Bitpos<36) && Receivebitinfo>2) TimeSignaldoubtfull = true; // If a date time bit is received and the received bit is not 0 or 1 then time becomes doubtfull
  if(PrintDebugInfo)
    {
     sprintf(sptext,"@%s%s%s", (MinOK<2?(MinOK?"M":"m"):"."),
                    (HourOK<2?(HourOK?"H":"h"):"."),(YearOK<2?(YearOK?"Y":"y"):"."));                         Tekstprint(sptext);
     sprintf(sptext,"%6ld %5ld %2ld%% [%d] %0.2d:%0.2d:%0.2d %0.2d-%0.2d-%0.4d/%0.1d F%ld OK:%d",
                     SumSecondSignal, SumSignalCounts, 100*SumSecondSignal/SumSignalCounts, Receivebitinfo, 
                     Dhour, Dminute, Bitpos, Dday, Dmonth, 2000+Dyear, Dwday, SignalFaults, TimeOK);          Tekstprintln(sptext);
     }     
// sprintf(sptext,"\%d%d%d%d%d%d%d%d%d%d/", SignalBin[0],SignalBin[1],SignalBin[2],SignalBin[3],SignalBin[4], SignalBin[5],SignalBin[6],SignalBin[7],SignalBin[8],SignalBin[9]);  Serial.println(sptext);
// for (int x=0; x<10; x++) SignalBin[x]=0;
 SumSecondSignal = SumSignalCounts = 0;
 Dsecond = Bitpos;
 Bitpos++; 
return(TimeOK);
}           
                             #endif DCFTINY
                             #endif defined DCFTINY || defined DCFMOD
//--------------------------------------------
// CLOCK Heart beat in LED
//--------------------------------------------
void heartbeat() 
{
 static byte hbval         = 128;    // Heartbeat initial intensity
 static byte hbdelta       = 10;     // Determines how fast heartbeat is
 static uint32_t last_time = 0;  
 unsigned long now = millis();
 if ((now - last_time) < 40)    return;
 last_time = now;
 if (hbval > 230 || hbval < 20 ) hbdelta = -hbdelta; 
 hbval += hbdelta;
 analogWrite(HeartbeatLED, hbval);
}
                        #ifdef ROTARYMOD
//--------------------- KY-040 rotary encoder ------------------------- 
//--------------------------------------------
// KY-040 ROTARY check if the rotary is moving
//--------------------------------------------
void RotaryEncoderCheck(void)
{
 long encoderPos = myEnc.read();
 if ( (unsigned long) (millis() - RotaryPressTimer) > 60000)        // After 60 sec after shaft is pressed time of light intensity can not be changed 
   {
    if (ChangeTime || ChangeLightIntensity)                         
      {
        Tekstprintln("<-- Changing time is over -->");
        NoofRotaryPressed = 0;
      }
    ChangeTime            = false;
    ChangeLightIntensity  = false;
   }  
 if (ChangeTime || ChangeLightIntensity)                            // If shaft is pressed time of light intensity can be changed
   {
    if ( encoderPos && ( (millis() - Looptime) > 200))               // If rotary turned avoid debounce within 0.2 sec
     {   
     Serial.print(F("----> Index:"));   Serial.println(encoderPos);
     if (encoderPos >0)                                             // Increase  MINUTES of light intensity
       {     
        if (ChangeLightIntensity)  { WriteLightReducer(5); }        // If time < 60 sec then adjust light intensity factor
        if (ChangeTime) 
          {
           if (NoofRotaryPressed == 1)                              // Change hours
              { if( ++Ihour >23) { Ihour = 0; } }      
           if (NoofRotaryPressed == 2)                              // Change minutes
              {  Isecond = 0;
               if( ++Iminute >59) { Iminute = 0; if( ++Ihour >23) { Ihour = 0; } }  }
           } 
        }    
      if (encoderPos <0)                                            // Increase the HOURS
       {
       if (ChangeLightIntensity)   { WriteLightReducer(-5); }    // If time < 60 sec then adjust light intensity factor
       if (ChangeTime)     
          {
           if (NoofRotaryPressed == 1)                              // Change hours
            { if( Ihour-- ==0) { Ihour = 23; } }      
           if (NoofRotaryPressed == 2)                              // Change minutes
            { Isecond = 0;
             if( Iminute-- == 0) { Iminute = 59; if( Ihour-- == 0) { Ihour = 23; } }  }
          }          
        } 
      SetRTCTime();  
      Print_RTC_tijd();
      myEnc.write(0);                                               // Set encoder pos back to 0
      Looptime = millis();       
     }                                                
   }
 if (digitalRead(clearButton) == LOW )                              // Set the time by pressing rotary button
   { 
    delay(250);
    ChangeTime            = false;
    ChangeLightIntensity  = false;
    SeeDCFsignalInDisplay = false;                                  // Shows the DCF-signal in the display
    RotaryPressTimer      = millis();                               // Record the time the shaft was pressed.
    if(++NoofRotaryPressed >10 ) NoofRotaryPressed = 0;
    switch (NoofRotaryPressed)                                      // No of times the rotary is pressed
      {
       case 1:  ChangeTime = true;            BlinkUUR(3, 20);      break; // Change the hours
       case 2:  ChangeTime = true;            BlinkHETISWAS(3, 20); break; // Change the hours        
       case 3:  ChangeLightIntensity = true;  BlinkLetters(5, 10);  break; // Turn on all LEDs and change intensity 
       case 4:  Mem.DisplayChoice = DEFAULTCOLOUR;                  break;
       case 5:  Mem.DisplayChoice = HOURLYCOLOUR;                   break;        
       case 6:  Mem.DisplayChoice = WHITECOLOR;                     break;
       case 7:  Mem.DisplayChoice = OWNCOLOUR;                      break;
       case 8:  Mem.DisplayChoice = OWNHETISCLR;                    break;
       case 9:  Mem.DisplayChoice = WHEELCOLOR;                     break;    
       case 10: Mem.DisplayChoice = DIGITAL;                        break;                                      
//                     #if defined DCFMOD || defined DCFTINY
//     case 11:  SeeDCFsignalInDisplay = true;                      break; // Show DCF signal   
//                     #endif DCFMOD || DCFTINY
       default: NoofRotaryPressed = 0; 
                Mem.DisplayChoice = DEFAULTCOLOUR;
                SeeDCFsignalInDisplay = ChangeTime = ChangeLightIntensity  = false;  
                Selftest();        
                break;                         
      }
    Serial.print(F("NoofRotaryPressed: "));   Serial.println(NoofRotaryPressed);   
    sprintf(sptext,"Display choice stored: Q%d", Mem.DisplayChoice);
    Tekstprintln(sptext);  
    myEnc.write(0);
    Looptime = millis();     
    Displaytime();  
   }
  myEnc.write(0);
 }
//--------------------------------------------
//  Blink UUR
//--------------------------------------------
void BlinkUUR(int NoofBlinks, int Delayms)
{
 for (int n=0 ; n<=NoofBlinks; n++) 
     {LedsOff(); Laatzien(); delay(Delayms); 
      UUR; Laatzien();       delay(Delayms);} 
}
//--------------------------------------------
//  Blink HET IS WAS
//--------------------------------------------
void BlinkHETISWAS (int NoofBlinks, int Delayms)
{
 for (int n=0 ; n<=NoofBlinks; n++) 
     {LedsOff(); Laatzien();    delay(Delayms); 
      HET; IS; WAS; Laatzien(); delay(Delayms);} 
}
//--------------------------------------------
//  Blink Letters
//--------------------------------------------
void BlinkLetters (int NoofBlinks, int Delayms)
{
 for (int n=0 ; n<=NoofBlinks; n++) 
     {LedsOff(); Laatzien(); delay(Delayms); 
      ColorLeds("",36,59,0XFF0000FF);  Laatzien(); delay(Delayms);} 
}
                          #endif ROTARYMOD

//--------------------------------------------
// CLOCK Self test sequence
//--------------------------------------------
void Selftest(void)
{ 
 //  Serial.print(F("Self test in : "));
 // start by clearing the display to a known state
  GetTijd(1); //Prints time in Serial monitor
  LedsOff(); 
  HET;   Laatzien(); IS;    Laatzien(); WAS;    Laatzien(); PRECIES; Laatzien(); MTIEN;  Laatzien();  MVIJF; Laatzien();    
  KWART; Laatzien(); VOOR;  Laatzien(); OVER;   Laatzien(); HALF;    Laatzien(); MIDDER; Laatzien(); VIJF;   Laatzien();
  TWEE;  Laatzien(); EEN;   Laatzien(); VIER;   Laatzien(); TIEN;    Laatzien(); TWAALF; Laatzien(); DRIE;   Laatzien();
  NEGEN; Laatzien(); NACHT; Laatzien(); ACHT;   Laatzien(); ZES;     Laatzien(); ZEVEN;  Laatzien();  ELF;   Laatzien(); 
  NOEN;  Laatzien(); UUR;   Laatzien(); EDSOFT; Laatzien();
  Tekstprintln("*");    

  IT;   Laatzien();  ISUK;    Laatzien(); WASUK; Laatzien(); EXACT; Laatzien(); HALFUK;  Laatzien(); TWENTY; Laatzien();    
  MFIVE; Laatzien(); QUARTER; Laatzien(); MTEN;  Laatzien(); PAST;  Laatzien(); TO;      Laatzien(); SIXUK;  Laatzien();
  TWO;  Laatzien();  FIVE;    Laatzien(); TWELVE;Laatzien(); TEN;   Laatzien(); ELEVEN;  Laatzien(); FOUR;   Laatzien();
  NINE; Laatzien();  THREE;  Laatzien();  EIGHT; Laatzien(); ONE;   Laatzien(); SEVEN;   Laatzien(); OCLOCK; Laatzien(); 
  Tekstprintln("*"); 

  Play_Lights();    
//  Displaytime();
}
// -------------------------- End Selftest

//--------------------------- Time functions --------------------------
void Displaytime(void)
{   
 LedsOff();                                 // start by clearing the display to a known state
 SetColours();                              // Set the colours for the chosen palette   
/*
 LEDNrCounter = 0;
 NoTextInColorLeds = true;  
 while(LEDNrCounter < 626)                
    {
    LEDNrCounter+=25;   // LEDs zigzaggen, formule voor uitwerken
    Dutch();  
    English();  
    German(); 
    French();
    ShowLeds();
    delay(500);
    LedsOff();
     }
  NoTextInColorLeds=false;  
  LedsOff();
  LEDNrCounter=0;
 */
 if (Mem.DisplayChoice == DIGITAL ) { TimePlaceDigit(Ihour,Iminute); }
 else
   {
   if(Mem.DisplayChoice == WHITECOLOR) MakeCross(CrossColour);
                     #ifdef NL
    Dutch();  //ShowLeds();
                     #endif NL
                     #ifdef UK
    English();   //ShowLeds();
                     #endif UK
                     #ifdef DE
    German();  //ShowLeds();
                     #endif DE
                     #ifdef FR
    French(); // ShowLeds();
                     #endif FR
   }
// Tekstprintln("");

 ShowLeds();
}

//--------------------------------------------
// DS3231 Get time from DS3231
//--------------------------------------------
void GetTijd(byte printit)
{
 Inow    = RTCklok.now();
 Ihour   = Inow.hour();
 Iminute = Inow.minute();
 Isecond = Inow.second();
 Iday    = Inow.day();
 Imonth  = Inow.month();
 Iyear   = Inow.year()-2000;
// if (Ihour > 24) { Ihour = random(12)+1; Iminute = random(60)+1; Isecond = 30;}  // set a time if time module is absent or defect
 if (printit)  Print_RTC_tijd(); 
}

//--------------------------------------------
// DS3231 utility function prints time to serial
//--------------------------------------------
void Print_RTC_tijd(void)
{
 sprintf(sptext,"%0.2d:%0.2d:%0.2d %0.2d-%0.2d-%0.4d",Inow.hour(),Inow.minute(),Inow.second(),Inow.day(),Inow.month(),Inow.year());
 Tekstprintln(sptext);
}
                    #ifdef LCDMOD
//--------------------------------------------
// CLOCK Print time to LCD display
//--------------------------------------------
void Print_tijd_LCD(void)
{
 lcd.home (); // set cursor to 0,0
 sprintf(sptext,"%0.2d:%0.2d:%0.2d",Inow.hour(),Inow.minute(),Inow.second());   lcd.print(sptext);
 sprintf(sptext," LDR%d   ",analogRead(PhotoCellPin));                          lcd.print(sptext);
 lcd.setCursor (0,1);        // go to start of 2nd line
 sprintf(sptext,"%0.2d-%0.2d-%0.4d",Inow.day(),Inow.month(),Inow.year());       lcd.print(sptext);
 sprintf(sptext," DCF%d   ",DCF_signal);                                        lcd.print(sptext);
}
                    #endif LCDMOD
//--------------------------------------------
// CLOCK utility function prints time to serial
//--------------------------------------------
void Print_tijd(void)
{
 sprintf(sptext,"%0.2d:%0.2d:%0.2d",Ihour,Iminute,Isecond);
 Tekstprintln(sptext);
}

//--------------------------------------------
// DS3231 Set time in module and print it
//--------------------------------------------
void SetRTCTime(void)
{ 
 Ihour   = constrain(Ihour  , 0,24);
 Iminute = constrain(Iminute, 0,59); 
 Isecond = constrain(Isecond, 0,59); 
 RTCklok.adjust(DateTime(Inow.year(), Inow.month(), Inow.day(), Ihour, Iminute, Isecond));
 GetTijd(0);                                     // synchronize time with RTC clock
 Displaytime();
 Print_tijd();
}
//--------------------------------------------
// DS3231 Get temperature from module
//--------------------------------------------
int Get3231Temp(void)
{
 byte tMSB, tLSB;
 int temp3231;
  
 Wire.beginTransmission(DS3231_I2C_ADDRESS);     // Temp registers (11h-12h) get updated automatically every 64s
 Wire.write(0x11);
 Wire.endTransmission();
 Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
 
 if(Wire.available()) 
  {
    tMSB = Wire.read();                          // 2's complement int portion
    tLSB = Wire.read();                          // fraction portion 
    temp3231 = (tMSB & B01111111);               // do 2's math on Tmsb
    temp3231 += ( (tLSB >> 6) * 0.25 ) + 0.5;    // only care about bits 7 & 8 and add 0.5 to round off to integer   
  }
 else {  temp3231 = -273; }   
 return (temp3231);
}
// ------------------- End  Time functions 

// --------------------Colour Clock Light functions -----------------------------------
//--------------------------------------------
//  LED Set color for LED
//--------------------------------------------
void ColorLeds(char* Tekst, int FirstLed, int LastLed, uint32_t RGBWColor)
{ 
 
// if(LEDNrCounter>0) FirstLed = ((1+(LEDNrCounter+FirstLed)%25)*50)-FirstLed;
  
 strip.fill(RGBWColor, FirstLed, ++LastLed-FirstLed );
 //for(int n = FirstLed; n <= LastLed; n++)  strip.setPixelColor(n,RGBWColor  ); Serial.println(RGBWColor,HEX);  
 if (!NoTextInColorLeds && strlen(Tekst) > 0 ){sprintf(sptext,"%s ",Tekst); Tekstprint(sptext); }   // Print the text  
}

void MakeCross(uint32_t RGBWColor)
{ 
  for(int n=12;n<613;n+=25) strip.fill(RGBWColor,n,1);
  strip.fill(RGBWColor, 300, 25);
}
//--------------------------------------------
//  LED Clear display settings of the LED's
//--------------------------------------------
void LedsOff(void) 
{ 
 strip.fill(0, 0, NUM_LEDS );                    // for (int n = 0; n <= NUM_LEDS; n++)  strip.setPixelColor(n,0); 
}

//--------------------------------------------
// LED Turn On and Off the LED's after Delaytime is milliseconds
//--------------------------------------------
void Laatzien()
{ 
 ShowLeds();
// delay(50);
 LedsOff(); 
}

//--------------------------------------------
//  LED Push data in LED strip to commit the changes
//--------------------------------------------
void ShowLeds(void)
{
 strip.show();
}
//--------------------------------------------
//  LED Set brighness of LEDs
//--------------------------------------------  
void SetBrightnessLeds( byte Bright)
{
 strip.setBrightness(Bright); 
 ShowLeds();
}

//--------------------------------------------
//  LED convert HSV to RGB 
//  h is from 0-360, s,v values are 0-1
//  r,g,b values are 0-255
//--------------------------------------------
uint32_t HSVToRGB(double H, double S, double V) 
{
  int i;
  double r, g, b, f, p, q, t;
  if (S == 0)  {r = V;  g = V;  b = V; }
  else
  {
    H >= 360 ? H = 0 : H /= 60;
    i = (int) H;
    f = H - i;
    p = V * (1.0 -  S);
    q = V * (1.0 - (S * f));
    t = V * (1.0 - (S * (1.0 - f)));
    switch (i) 
    {
     case 0:  r = V;  g = t;  b = p;  break;
     case 1:  r = q;  g = V;  b = p;  break;
     case 2:  r = p;  g = V;  b = t;  break;
     case 3:  r = p;  g = q;  b = V;  break;
     case 4:  r = t;  g = p;  b = V;  break;
     default: r = V;  g = p;  b = q;  break;
    }
  }
return FuncCRGBW((int)(r*255), (int)(g*255), (int)(b*255), 0 );      // R, G, B, W 
}
//--------------------------------------------
//  LED function to make RGBW color
//-------------------------------------------- 
uint32_t FuncCRGBW( uint32_t Red, uint32_t Green, uint32_t Blue, uint32_t White)
{ 
 return ( (White<<24) + (Red << 16) + (Green << 8) + Blue );
}
//--------------------------------------------
//  LED functions to extract RGBW colors
//-------------------------------------------- 
 uint8_t Cwhite(uint32_t c) { return (c >> 24);}
 uint8_t Cred(  uint32_t c) { return (c >> 16);}
 uint8_t Cgreen(uint32_t c) { return (c >> 8); }
 uint8_t Cblue( uint32_t c) { return (c);      }


//--------------------------------------------
//  LED Set second color
//  Set the colour per second of 'IS' and 'WAS'
//--------------------------------------------
void SetColours(void)
{  
 switch (Mem.DisplayChoice)
  {
   case DEFAULTCOLOUR: LetterColor   = DefaultColor; 
                       UKLetterColor = UKDefaultColor;
                       DELetterColor = DEDefaultColor;
                       FRLetterColor = FRDefaultColor;           break; // HET IS WAS changing   
   case HOURLYCOLOUR : LetterColor   = HourColor[Ihour];
                       UKLetterColor = HourColor[(Ihour+1)%24];
                       DELetterColor = HourColor[(Ihour+2)%24];
                       FRLetterColor = HourColor[(Ihour+3)%24];  break; // A colour every hour   
   case WHITECOLOR   : LetterColor   = WhiteColour; 
                       UKLetterColor = WhiteColour;
                       DELetterColor = WhiteColour;
                       FRLetterColor = WhiteColour;              break; // all white
   case OWNCOLOUR    : LetterColor   = Mem.OwnColour; 
                       UKLetterColor = Mem.OwnColour;
                       DELetterColor = Mem.OwnColour;
                       FRLetterColor = Mem.OwnColour;            break;
   case OWNHETISCLR  : LetterColor   = Mem.OwnColour; 
                       UKLetterColor = Mem.OwnColour;
                       DELetterColor = Mem.OwnColour;
                       FRLetterColor = Mem.OwnColour;            break; // own colour HET IS WAS changing  
   case WHEELCOLOR   : LetterColor   = Wheel((Iminute*4));       
                       UKLetterColor = Wheel((60+Iminute*4));
                       DELetterColor = Wheel((120+Iminute*4));
                       FRLetterColor = Wheel((180+Iminute*4));   break; // Colour of all letters changes per second
   case DIGITAL      : LetterColor = white;                      break; // digital display of time. No IS WAS turn color off in display
  }
}
//--------------------------------------------
//  LED Dim the leds by PWM measured by the LDR and print values
//--------------------------------------------                                                                                           
void DimLeds(byte print) 
{                                                                                                      
  int LDRreading = analogRead(PhotoCellPin);
  int LDR_read = (4 * Previous_LDR_read + LDRreading) / 5;                        // Read lightsensor 0 - 1024 bits 
  Previous_LDR_read = LDR_read;
  OutPhotocell = (int)((Mem.LightReducer * sqrt(63.5*LDR_read))/100);             // 0-255 bits.  Linear --> hyperbolic with sqrt
  MinPhotocell = MinPhotocell > LDR_read ? LDR_read : MinPhotocell;
  MaxPhotocell = MaxPhotocell < LDR_read ? LDR_read : MaxPhotocell;
  BrightnessCalcFromLDR = (byte)constrain(OutPhotocell, Mem.LowerBrightness, 255);// filter out of strange results 
  if(print)
  {
   int Percent = (int)(100*BrightnessCalcFromLDR/255);
   sprintf(sptext,"Sensor:%3d",LDRreading);         Tekstprint(sptext);
   sprintf(sptext," Min:%3d",MinPhotocell);         Tekstprint(sptext);
   sprintf(sptext," Max:%3d",MaxPhotocell);         Tekstprint(sptext);
   sprintf(sptext," Out:%3d",OutPhotocell);         Tekstprint(sptext);
   sprintf(sptext,"=%2d%%",Percent);                 Tekstprint(sptext);
   sprintf(sptext," Temp:%2dC ", Get3231Temp()-2);  Tekstprint(sptext);
   Print_tijd(); 
  }
 if(LEDsAreOff) SetBrightnessLeds(0);                                              // Turn off all LEDs
 else           SetBrightnessLeds(BrightnessCalcFromLDR); 
}
//--------------------------------------------
//  LED Turn On en Off the LED's
//--------------------------------------------
void Play_Lights()
{
  WhiteOverRainbow(1, 5, 5 );  // wait, whiteSpeed, whiteLength
  LedsOff();
}

//--------------------------------------------
//  LED Wheel
//  Input a value 0 to 255 to get a color value.
//  The colours are a transition r - g - b - back to r.
//--------------------------------------------
uint32_t Wheel(byte WheelPos) 
{
 WheelPos = 255 - WheelPos;
 if(WheelPos < 85)   { return FuncCRGBW( 255 - WheelPos * 3, 0, WheelPos * 3, 0);  }
 if(WheelPos < 170)  { WheelPos -= 85;  return FuncCRGBW( 0,  WheelPos * 3, 255 - WheelPos * 3, 0); }
 WheelPos -= 170;      
 return FuncCRGBW(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

//--------------------------------------------
//  LED RainbowCycle
// Slightly different, this makes the rainbow equally distributed throughout
//--------------------------------------------
void RainbowCycle(uint8_t wait) 
{
  uint16_t i, j;
  for(j=0; j<256 * 5; j++) 
  {                                                       // 5 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++)  ColorLeds("",i,i,Wheel(((i * 256 / NUM_LEDS) + j) & 255));
    ShowLeds();
    delay(wait);
  }
}

//--------------------------------------------
//  LED WhiteOverRainbow
//--------------------------------------------
void WhiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) 
{
  if(whiteLength >= NUM_LEDS) whiteLength = NUM_LEDS - 1;
  int head = whiteLength - 1;
  int tail = 0;
  int loops = 1;
  int loopNum = 0;
  static unsigned long lastTime = 0;
  while(true)
  {
    for(int j=0; j<256; j++) 
     {
      for(uint16_t i=0; i<NUM_LEDS; i++) 
       {
        if((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) )
             // strip.setPixelColor(i, strip.Color(0,0,0, 255 ) );
              ColorLeds("",i,i,0XFFFFFF );
        else  //strip.setPixelColor(i, Wheel(((i * 256 / NUM_LEDS) + j) & 255));
        ColorLeds("",i,i,Wheel(((i * 256 / NUM_LEDS) + j) & 255));
       }
      if(millis() - lastTime > whiteSpeed) 
       {
        head++;        tail++;
        if(head == NUM_LEDS) loopNum++;
        lastTime = millis();
      }
      if(loopNum == loops) return;
      head %= NUM_LEDS;
      tail %= NUM_LEDS;
      ShowLeds();
      delay(wait);
    }
  }  // end while
}


//  LED Place digits 0 - 9 in Matrix display
// First row and column = 0, PosX,PosY is left top position of 3x5 digit
// Calculate position LED #define MATRIX_WIDTH 12 #define MATRIX_HEIGHT 12
//--------------------------------------------
void Zet_Pixel(byte Cijfer,byte Pos_X, byte Pos_Y) 
{ 
 uint32_t LEDnum;
 uint32_t OrgColor;
// CheckColourStatus();
 for(int i=0;i<3;i++)
  {  
   for(int j=0;j<5;j++)
   {
    int c = pgm_read_byte_near ( &Getal[Cijfer][i][j]); //The pgm_read_byte_near() is a macro that reads a byte of data stored in a specified address(PROGMEM area).
    if ( c )          // if Digit == 1 then turn that light on
     {                // Serial.print(strip.getPixelColor(LEDnum) & 0X00FFFFFF,HEX); Serial.print(" ");
      if((Pos_Y+j)%2) LEDnum = ((MATRIX_WIDTH -1) - (Pos_X + i) + (Pos_Y + j) * (MATRIX_HEIGHT));
      else            LEDnum =                      (Pos_X + i) + (Pos_Y + j) * (MATRIX_HEIGHT); 
      strip.getPixelColor(LEDnum) && white ? OrgColor = LetterColor : OrgColor = 0;
      ColorLeds("",  LEDnum, LEDnum, (uint32_t)(OrgColor + white));
      (Pos_Y + j) * (MATRIX_HEIGHT);  
     }
   }
 }
}
//--------------------------------------------
//  LED Time in for digits in display
//--------------------------------------------
void TimePlaceDigit(byte uur, byte minuut)
{   
 Zet_Pixel(    uur / 10, 2, 1);  Zet_Pixel(    uur % 10, 7, 1);
 Zet_Pixel( minuut / 10, 2, 7);  Zet_Pixel( minuut % 10, 7, 7);
}

//--------------------------------------------
//  LED In- or decrease light intensity value
//--------------------------------------------
void WriteLightReducer(int amount)
{
 int value = Mem.LightReducer + amount;                 //Prevent byte overflow by making it an integer before adding
 Mem.LightReducer = (byte) constrain (value, 0 , 255);  // May not be larger than 255
 sprintf(sptext,"Max brightness: %3d%%",Mem.LightReducer);
 Tekstprintln(sptext);
}

//--------------------------------------------
//  LED Write lowest allowable light intensity to EEPROM
//--------------------------------------------
void WriteLowerBrightness(byte waarde)
{
 Mem.LowerBrightness = constrain (waarde, 0 , 255);      // Range between 0 and 255
 sprintf(sptext,"Lower brightness: %3d bits", Mem.LowerBrightness);
 Tekstprintln(sptext);
}

// --------------------End Light functions 


//--------------------------------------------
//  CLOCK Input from Bluetooth or Serial
//--------------------------------------------
void ReworkInputString(String InputString)
{
 String temp;
 InputString.toCharArray(sptext, MAXTEXT-1);
// Tekstprintln(sptext);
  Serial.println(InputString);
  if (InputString.length() >10) return;

 if(  InputString[0] > 64 && InputString[0] <123 )                     // Does the string start with a letter?
  { 
//   Tekstprintln(sptext);
   switch ((byte)InputString[0])
   {
    case 'A':
    case 'a':
            if (InputString.length() == 1)
              {
               PrintDebugInfo = 1 - PrintDebugInfo;
               sprintf(sptext,"See DCF debug info: %s",PrintDebugInfo ? "On" : "Off");
               Tekstprintln(sptext);
              }
             else Tekstprintln("**** Length fault. Enter A ****");
             break;  
    case '0':
             int i,n;         // i = 0,1,2 -> DCFtiny, DCF77,Both
             if(InputString.length() == 1)
               {
                Mem.LightReducer    = 40;
                Mem.LowerBrightness = 5;
                Mem.DisplayChoice = Mem.TurnOffLEDsAtHH = Mem.TurnOnLEDsAtHH = Mem.OwnColour = 0;
                Mem.EdMinTijd = Mem.EdMin = Mem.ThMin=Mem.EdTh = Mem.EdThMin = Mem.ThWrong = 0;
                Mem.EdWrong = Mem.ValidTimes = Mem.MinutesSinceStart = 0;       
//                Tekstprintln("Statistics data erased. EEPROM data will be cleared at midnight"); 
               }
              if(InputString.length() == 3)
               {
                for (i=0 ; i<EEPROM.length(); i++) { EEPROM.write(i, 0); }
                Tekstprintln("EEPROM data were erased"); 
                setup();
               }
                                                    break;
    case 'C':
    case 'c':  
             if (InputString.length() == 9 )
               {
                temp = InputString.substring(1,9);
                LetterColor = Mem.OwnColour = HexToDec(temp);               // Display letter color 
                sprintf(sptext,"Cross colour stored0X%X", Mem.OwnColour);
                Tekstprintln(sptext); 
                Tekstprintln("**** Cross colour changed ****"); 
                Displaytime();
               }
             else Tekstprintln("**** Length fault. Enter Pwwrrggbb ****");            
             break;
    case 'D':
    case 'd':  
            if (InputString.length() == 9 )
              {
               int Jaar;
               temp   = InputString.substring(1,3);     Iday = (byte) temp.toInt(); 
               temp   = InputString.substring(3,5);   Imonth = (byte) temp.toInt(); 
               temp   = InputString.substring(5,9);     Jaar =  temp.toInt(); 
               Iday   = constrain(Iday  , 0, 31);
               Imonth = constrain(Imonth, 0, 12); 
               Jaar   = constrain(Jaar , 1000, 9999); 
               RTCklok.adjust(DateTime(Jaar, Imonth, Iday, Inow.hour(), Inow.minute(), Inow.second()));
               sprintf(sptext,"%0.2d:%0.2d:%0.2d %0.2d-%0.2d-%0.4d",Inow.hour(),Inow.minute(),Inow.second(),Iday,Imonth,Jaar);
               Tekstprintln(sptext);
               ResetArduino(); // Reset Arduino because of DCFtiny
              }
            else Tekstprintln("**** Length fault. Enter ddmmyyyy ****");
            break;

    case 'G':                                                         // Toggle DCF Signal on Display
    case 'g':
            if (InputString.length() == 1)
              {
               SeeDCFsignalInDisplay = 1 - SeeDCFsignalInDisplay;
               sprintf(sptext,"SeeDCFsignal:%s",SeeDCFsignalInDisplay ? "On" : "Off");
               Tekstprintln(sptext);
               }
             else Tekstprintln("**** Length fault. Enter G ****");
             break;
    case 'H':                                                         // Toggle DCF Signal on Display
    case 'h':
            if (InputString.length() == 1)
              {
               UseDCF = 1 - UseDCF;
               sprintf(sptext,"Use DCF-receiver:%s",UseDCF ? "On" : "Off");
               Tekstprintln(sptext);
               }
             else Tekstprintln("**** Length fault. Enter H ****");               
             break; 
    case 'I':
    case 'i': 
            if (InputString.length() == 1)
            {  
             SWversion();
            }
            break;
    case 'L':                                                         // Lowest value for Brightness
    case 'l':
             if (InputString.length() < 5)
               {      
 //               temp = InputString.substring(1);
                Mem.LowerBrightness   = (byte) constrain(InputString.substring(1).toInt(),0,255);
                sprintf(sptext,"Lower brightness changed to: %d bits",Mem.LowerBrightness);
                Tekstprintln(sptext);
               }
             else Tekstprintln("**** Length fault. Enter Lnnn ****");
             break;
    case 'M':                                                         // factor ( 0 - 1) to multiply brighness (0 - 255) with 
    case 'm':
             if (InputString.length() < 5)
               {    
                temp = InputString.substring(1);
                Mem.LightReducer = constrain(temp.toInt(),1,255);
                sprintf(sptext,"Max brightness changed to: %d%%",Mem.LightReducer);
                Tekstprintln(sptext);
               }
             else Tekstprintln("**** Length fault. Enter Mnnn ****");
              break;
    case 'N':
    case 'n':
             if (InputString.length() == 1 )         Mem.TurnOffLEDsAtHH = Mem.TurnOnLEDsAtHH = 0;
             if (InputString.length() == 5 )
              {
               temp   = InputString.substring(1,3);  Mem.TurnOffLEDsAtHH = (byte) temp.toInt(); 
               temp   = InputString.substring(3,5);  Mem.TurnOnLEDsAtHH = (byte) temp.toInt(); 
              }
             Mem.TurnOffLEDsAtHH = constrain(Mem.TurnOffLEDsAtHH, 0, 23);
             Mem.TurnOnLEDsAtHH  = constrain(Mem.TurnOnLEDsAtHH,  0, 23); 
             sprintf(sptext,"LEDs are OFF between %2d:00 and %2d:00", Mem.TurnOffLEDsAtHH,Mem.TurnOnLEDsAtHH );
             Tekstprintln(sptext); 
                                                                   break;
    case 'O':
    case 'o':
             if(InputString.length() == 1)
               {
                LEDsAreOff = !LEDsAreOff;
                sprintf(sptext,"LEDs are %s", LEDsAreOff?"OFF":"ON" );
                Tekstprintln(sptext);
                if(LEDsAreOff) { LedsOff(); ShowLeds();}               // Turn the LEDs off
                else {DimLeds(true); Displaytime();}                   // Turn the LEDs on                
               }
                                                                  break;                                                                   
    case 'P':
    case 'p':  
             if (InputString.length() == 9 )
               {
                temp = InputString.substring(1,9);
                LetterColor = Mem.OwnColour = HexToDec(temp);               // Display letter color 
                sprintf(sptext,"Own colour stored0X%X", Mem.OwnColour);
                Tekstprintln(sptext); 
                Tekstprintln("**** Own colour changed ****"); 
                Displaytime();
               }
             else Tekstprintln("**** Length fault. Enter Pwwrrggbb ****");            
             break;
    case 'q':
    case 'Q':  
             if (InputString.length() == 2 )
               {
                temp   = InputString.substring(1,2);     
                 Mem.DisplayChoice = (byte) temp.toInt(); 
                sprintf(sptext,"Display choice: Q%d", Mem.DisplayChoice);
                Tekstprintln(sptext);
                lastminute = 99;                                      // Force a minute update
               }
             else Tekstprintln("**** Display choice length fault. Enter Q0 - Q6"); 
             SetColours();                                            // Set the colours for this palette
             Displaytime();                                           // Turn on the LEDs with proper time             
            break;     


    case 'R':
    case 'r':
            if (InputString.length() == 1)
              {
               Reset();                                               // Reset all settings 
               Tekstprintln("**** Resetted to default settings ****"); 
              }
            break;
    case 'S':
    case 's':
             if (InputString.length() == 1)
               {   
                Zelftest = 1 - Zelftest; 
               sprintf(sptext,"Zelftest: %d",Zelftest);
               Tekstprintln(sptext); 
               Displaytime();                                          // Turn on the LEDs with proper time
               }                                
             else Tekstprintln("**** Length fault. Enter S ****");
             break; 
    case 'T':
    case 't':

             if(InputString.length() == 7)  // T125500
              {
               temp = InputString.substring(1,3);   
               if(temp.toInt() <24) Ihour = temp.toInt();
               else break; 
               temp = InputString.substring(3,5);   
               if(temp.toInt() <60) Iminute = temp.toInt();
               else break;                
               temp = InputString.substring(5,7);   
               if(temp.toInt() <60) Isecond = temp.toInt();
               else break;
               SetRTCTime();
              }
             else Tekstprintln("**** Length fault. Enter Thhmmss ****");
              break;  
    case 'W':
    case 'w':
             if (InputString.length() >1) break;   
             TestLDR = 1 - TestLDR;                                   // If TestLDR = 1 LDR reading is printed every second instead every 30s
             sprintf(sptext,"TestLDR every second: %s",TestLDR? "On" : "Off");  Tekstprintln(sptext);
             break;    
    case 'X':
    case 'x':    
             if (InputString.length() >1 && InputString.length() < 6 )
               {
                temp = InputString.substring(1,5);
                MilliSecondValue = temp.toInt();                
               }
             Demo = 1 - Demo;                                          // Toggle Demo mode
             if (!Demo)  
              { 
                MilliSecondValue = 1000;                               // So clock runs again at normal speed minus 1 ms
              }
             sprintf(sptext,"Demo mode: %d MillisecondTime=%d",Demo,MilliSecondValue);
             Tekstprintln(sptext);
             break;        
    default:
            break;
    }
  }
 else if (InputString.length() == 6 )    // for compatibility
  {
   temp = InputString.substring(0,2);   
   if(temp.toInt() <24) Ihour = temp.toInt(); 
   temp = InputString.substring(2,4);   
   if(temp.toInt() <60) Iminute = temp.toInt(); 
   temp = InputString.substring(4,6);   
   if(temp.toInt() <60) Isecond = temp.toInt(); 
   SetRTCTime();
  }
 EEPROM.put(0,Mem);                                                    // Update EEPROM                                                
 InputString = "";
 temp = "";
}
//--------------------------------------------
//  CLOCK Convert Hex to uint32
//--------------------------------------------
uint32_t HexToDec(String hexString) 
{
 uint32_t decValue = 0;
 int nextInt;
 for (int i = 0; i < hexString.length(); i++) 
  {
   nextInt = int(hexString.charAt(i));
   if (nextInt >= 48 && nextInt <= 57)  nextInt = map(nextInt, 48, 57, 0, 9);
   if (nextInt >= 65 && nextInt <= 70)  nextInt = map(nextInt, 65, 70, 10, 15);
   if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
   nextInt = constrain(nextInt, 0, 15);
   decValue = (decValue * 16) + nextInt;
  }
return decValue;
}
                      #ifdef NL
//--------------------------------------------
//  CLOCK Dutch clock display
//--------------------------------------------
void Dutch(void)
{
 if (Ihour == 12 && Iminute == 0 && random(10)==0) { HET; IS; NOEN; return; }
 if (Ihour == 00 && Iminute == 0 && random(5)==0) { HET; IS; MIDDER; NACHT; return; } 

 HET;                                       // HET light is always on
 switch (Iminute)
 {
  case  0: IS;  PRECIES; break;
  case  1: IS;  break;
  case  2: 
  case  3: WAS; break;
  case  4: 
  case  5: 
  case  6: IS;  MVIJF; OVER; break;
  case  7: 
  case  8: WAS; MVIJF; OVER; break;
  case  9: 
  case 10: 
  case 11: IS;  MTIEN; OVER; break;
  case 12: 
  case 13: WAS; MTIEN; OVER; break;
  case 14: 
  case 15: 
  case 16: IS;  KWART; OVER; break;
  case 17: 
  case 18: WAS; KWART; OVER; break;
  case 19: 
  case 20: 
  case 21: IS;  MTIEN; VOOR; HALF; break;
  case 22: 
  case 23: WAS; MTIEN; VOOR; HALF; break;
  case 24: 
  case 25: 
  case 26: IS;  MVIJF; VOOR; HALF; break;
  case 27: 
  case 28: WAS; MVIJF; VOOR; HALF; break;
  case 29: IS;  HALF; break;
  case 30: IS;  PRECIES; HALF; break;
  case 31: IS;  HALF; break;
  case 32: 
  case 33: WAS; HALF; break;
  case 34: 
  case 35: 
  case 36: IS;  MVIJF; OVER; HALF; break;
  case 37: 
  case 38: WAS; MVIJF; OVER; HALF; break;
  case 39: 
  case 40: 
  case 41: IS;  MTIEN; OVER; HALF; break;
  case 42: 
  case 43: WAS; MTIEN; OVER; HALF; break;
  case 44: 
  case 45: 
  case 46: IS;  KWART; VOOR; break;
  case 47: 
  case 48: WAS; KWART; VOOR; break;
  case 49: 
  case 50: 
  case 51: IS;  MTIEN; VOOR;  break;
  case 52: 
  case 53: WAS; MTIEN; VOOR;  break;
  case 54: 
  case 55: 
  case 56: IS;  MVIJF; VOOR; break;
  case 57: 
  case 58: WAS; MVIJF; VOOR; break;
  case 59: IS;  break;
}
//if (Ihour >=0 && hour <12) digitalWrite(AMPMpin,0); else digitalWrite(AMPMpin,1);

 sayhour = Ihour;
 if (Iminute > 18 )  sayhour = Ihour+1;
 if (sayhour == 24) sayhour = 0;

switch (sayhour)
 {
  case 13:  
  case 1: EEN; break;
  case 14:
  case 2: TWEE; break;
  case 15:
  case 3: DRIE; break;
  case 16:
  case 4: VIER; break;
  case 17:
  case 5: VIJF; break;
  case 18:
  case 6: ZES; break;
  case 19:
  case 7: ZEVEN; break;
  case 20:
  case 8: ACHT; break;
  case 21:
  case 9: NEGEN; break;
  case 22:
  case 10: TIEN; break;
  case 23:
  case 11: ELF; break;
  case 0:
  case 12: TWAALF; break;
 } 
 switch (Iminute)
 {
  case 59: 
  case  0: 
  case  1: 
  case  2: 
  case  3: UUR;  break; 
 }
}
                      #endif NL
                      #ifdef UK
//--------------------------------------------
//  CLOCK English clock display
//--------------------------------------------
void English(void)
{
 IT;                                       // HET light is always on
 switch (Iminute)
 {
  case  0: ISUK;  EXACTUK; break;
  case  1: ISUK;  break;
  case  2: 
  case  3: WASUK; break;
  case  4: 
  case  5: 
  case  6: ISUK;  MFIVE; PAST; break;
  case  7: 
  case  8: WASUK; MFIVE; PAST; break;
  case  9: 
  case 10: 
  case 11: ISUK;  MTEN; PAST; break;
  case 12: 
  case 13: WASUK; MTEN; PAST; break;
  case 14: 
  case 15: 
  case 16: ISUK;  QUARTER; PAST; break;
  case 17: 
  case 18: WASUK; QUARTER; PAST; break;
  case 19: 
  case 20: 
  case 21: ISUK;  TWENTY; PAST; break;
  case 22: 
  case 23: WASUK; TWENTY; PAST; break;
  case 24: 
  case 25: 
  case 26: ISUK;  TWENTY; MFIVE; PAST; break;
  case 27: 
  case 28: WASUK; TWENTY; MFIVE; PAST; break;
  case 29: ISUK;  HALFUK; PAST; break;
  case 30: ISUK;  EXACTUK; HALFUK; PAST; break;
  case 31: ISUK;  HALFUK; PAST; break;
  case 32: 
  case 33: WASUK; HALFUK; PAST; break;
  case 34: 
  case 35: 
  case 36: ISUK;  TWENTY; MFIVE; TO; break;
  case 37: 
  case 38: WASUK; TWENTY; MFIVE; TO; break;
  case 39: 
  case 40: 
  case 41: ISUK;  TWENTY; TO; break;
  case 42: 
  case 43: WASUK; TWENTY; TO break;
  case 44: 
  case 45: 
  case 46: ISUK;  QUARTER; TO; break;
  case 47: 
  case 48: WASUK; QUARTER; TO; break;
  case 49: 
  case 50: 
  case 51: ISUK;  MTEN; TO;  break;
  case 52: 
  case 53: WASUK; MTEN; TO;  break;
  case 54: 
  case 55: 
  case 56: ISUK;  MFIVE; TO; break;
  case 57: 
  case 58: WASUK; MFIVE; TO; break;
  case 59: ISUK;  break;
}
//if (Ihour >=0 && hour <12) digitalWrite(AMPMpin,0); else digitalWrite(AMPMpin,1);

 sayhour = Ihour;
 if (Iminute > 33 ) sayhour = Ihour+1;
 if (sayhour == 24) sayhour = 0;

switch (sayhour)
 {
  case 13:  
  case 1:  ONE; break;
  case 14:
  case 2:  TWO; break;
  case 15:
  case 3:  THREE; break;
  case 16:
  case 4:  FOUR; break;
  case 17:
  case 5:  FIVE; break;
  case 18:
  case 6:  SIXUK; break;
  case 19:
  case 7:  SEVEN; break;
  case 20:
  case 8:  EIGHT; break;
  case 21:
  case 9:  NINE; break;
  case 22:
  case 10: TEN; break;
  case 23:
  case 11: ELEVEN; break;
  case 0:
  case 12: TWELVE; break;
 } 
 switch (Iminute)
 {
  case 59: 
  case  0: 
  case  1: 
  case  2: 
  case  3: OCLOCK;  break; 
 }

}
                           #endif UK
                           #ifdef DE
//--------------------------------------------
//  CLOCK German clock display
//--------------------------------------------
void German(void)
{
  ES;                                       // HET light is always on
 switch (Iminute)
 {
  case  0: IST;  GENAU; break;
  case  1: IST; KURZ; NACH; break;
  case  2: 
  case  3: WAR; break;
  case  4: 
  case  5: 
  case  6: IST; MFUNF; NACH; break;
  case  7: 
  case  8: WAR; MFUNF; NACH; break;
  case  9: 
  case 10: 
  case 11: IST; MZEHN; NACH; break;
  case 12: 
  case 13: WAR; MZEHN; NACH; break;
  case 14: 
  case 15: 
  case 16: IST; VIERTEL; NACH; break;
  case 17: 
  case 18: WAR; VIERTEL; NACH; break;
  case 19: 
  case 20: 
  case 21: IST; MZEHN; VOR; HALB; break;
  case 22: 
  case 23: WAR; MZEHN; VOR; HALB; break;
  case 24: 
  case 25: 
  case 26: IST; MFUNF; VOR; HALB; break;
  case 27: 
  case 28: WAR; MFUNF; VOR; HALB; break;
  case 29: IST; KURZ;  VOR; HALB; break;
  case 30: IST; GENAU; HALB; break;
  case 31: IST; KURZ;  NACH; HALB; break;
  case 32: 
  case 33: WAR; HALB; break;
  case 34: 
  case 35: 
  case 36: IST; MFUNF; NACH; HALB; break;
  case 37: 
  case 38: WAR; MFUNF; NACH; HALB; break;
  case 39: 
  case 40: 
  case 41: IST; MZEHN; NACH; HALB; break;
  case 42: 
  case 43: WAR; MZEHN; NACH; HALB; break;
  case 44: 
  case 45: 
  case 46: IST; VIERTEL; VOR; break;
  case 47: 
  case 48: WAR; VIERTEL; VOR; break;
  case 49: 
  case 50: 
  case 51: IST; MZEHN; VOR;  break;
  case 52: 
  case 53: WAR; MZEHN; VOR;  break;
  case 54: 
  case 55: 
  case 56: IST; MFUNF; VOR; break;
  case 57: 
  case 58: WAR; MFUNF; VOR; break;
  case 59: IST;  break;
}
//if (Ihour >=0 && hour <12) digitalWrite(AMPMpin,0); else digitalWrite(AMPMpin,1);

 sayhour = Ihour;
 if (Iminute > 18 ) sayhour = Ihour+1;
 if (sayhour == 24) sayhour = 0;

switch (sayhour)
 {
  case 13:  
  case 1: EINS; break;
  case 14:
  case 2: ZWEI; break;
  case 15:
  case 3: DREI; break;
  case 16:
  case 4: VIERDE; break;
  case 17:
  case 5: FUNF; break;
  case 18:
  case 6: SECHS; break;
  case 19:
  case 7: SIEBEN; break;
  case 20:
  case 8: ACHTDE; break;
  case 21:
  case 9: NEUN; break;
  case 22:
  case 10: ZEHN; break;
  case 23:
  case 11: ELFDE; break;
  case 0:
  case 12: ZWOLF; break;
 } 
 switch (Iminute)
 {
  case 59: 
  case  0: 
  case  1: 
  case  2: 
  case  3: UHR;  break; 
 }
}
                      #endif DE
                      #ifdef FR
//--------------------------------------------
//  CLOCK French clock display
//--------------------------------------------
void French(void)
{
 IL;                                       // HET light is always on
 switch (Iminute)
 {
  case  0: EST;   EXACT; DITLEHEURE; break;
  case  1: EST;   DITLEHEURE; break;
  case  2: 
  case  3: ETAIT; DITLEHEURE; break;
  case  4: 
  case  5: 
  case  6: EST;   DITLEHEURE; MCINQ; break;
  case  7: 
  case  8: ETAIT; DITLEHEURE; MCINQ; break;
  case  9: 
  case 10: 
  case 11: EST;   DITLEHEURE; MDIX;  break;
  case 12: 
  case 13: ETAIT; DITLEHEURE; MDIX;  break;
  case 14: 
  case 15: 
  case 16: EST;   DITLEHEURE; ET; QUART; break;
  case 17: 
  case 18: ETAIT; DITLEHEURE; ET; QUART; break;
  case 19: 
  case 20: 
  case 21: EST;   DITLEHEURE; VINGT; break;
  case 22: 
  case 23: ETAIT; DITLEHEURE; VINGT; break;
  case 24: 
  case 25: 
  case 26: EST;   DITLEHEURE; VINGT; MCINQ; break;
  case 27: 
  case 28: ETAIT; DITLEHEURE; VINGT; MCINQ; break;
  case 29: EST;   DITLEHEURE; ET; DEMI; break;
  case 30: EST;   EXACT; DITLEHEURE;  ET; DEMI; break;
  case 31: EST;   DITLEHEURE; ET; DEMI; break;
  case 32: 
  case 33: ETAIT; DITLEHEURE; ET; DEMI; break;
  case 34: 
  case 35: 
  case 36: EST;   DITLEHEURE; MOINS; VINGT; MCINQ; break;
  case 37: 
  case 38: ETAIT; DITLEHEURE; MOINS; VINGT; MCINQ; break;
  case 39: 
  case 40: 
  case 41: EST;   DITLEHEURE; MOINS; VINGT;  break;
  case 42: 
  case 43: ETAIT; DITLEHEURE; MOINS; VINGT;  break;
  case 44: 
  case 45: 
  case 46: EST;   DITLEHEURE; MOINS; LE; QUART; break;
  case 47: 
  case 48: ETAIT; DITLEHEURE; MOINS; LE; QUART; break;
  case 49: 
  case 50: 
  case 51: EST;   DITLEHEURE; MOINS; MDIX;   break;
  case 52: 
  case 53: ETAIT; DITLEHEURE; MOINS; MDIX;   break;
  case 54: 
  case 55: 
  case 56: EST;   DITLEHEURE; MOINS; MCINQ;  break;
  case 57: 
  case 58: ETAIT; DITLEHEURE; MOINS; MCINQ;  break;
  case 59: EST;   DITLEHEURE;  break;
 }
//if (Ihour >=0 && hour <12) digitalWrite(AMPMpin,0); else digitalWrite(AMPMpin,1);
// switch (Iminute)
// {
//  case 59: 
//  case  0: 
//  case  1: 
//  case  2: 
//  case  3: if(sayhour%12 == 1) {HEURE;} 
//           else                {HEURES;}  
//           break; 
// }
}

void DitLeHeure(void)
{
 byte sayhour = Ihour;
 if (Iminute > 33 ) sayhour = Ihour+1;
 if (sayhour == 24) sayhour = 0;

switch (sayhour)
 {
  case 13:  
  case 1:  UNE;    HEURE;  break;
  case 14:
  case 2:  DEUX;   HEURES;  break;
  case 15:
  case 3:  TROIS;  HEURES;  break;
  case 16:
  case 4:  QUATRE; HEURES; break;
  case 17:
  case 5:  CINQ;   HEURES;   break;
  case 18:
  case 6:  SIX;    HEURES;   break;
  case 19:
  case 7:  SEPT;   HEURES;  break;
  case 20:
  case 8:  HUIT;   HEURES; break;
  case 21:
  case 9:  NEUF;   HEURES; break;
  case 22:
  case 10: DIX;    HEURES; break;
  case 23:
  case 11: ONZE;   HEURES; break;
  case 0:  MINUIT; break;
  case 12: MIDI;   break;
 } 
}
                      #endif FR
                      #if defined(ARDUINO_SAMD_MKRWIFI1010)
//--------------------------------------------
// SETUP  ARDUINO_SAMD_MKRWIFI1010
//-------------------------------------------- 
void Setup_MKRWIFI1010()
{                                              
  pinMode(led, OUTPUT);                        // set the LED pin mode
  if (WiFi.status() == WL_NO_MODULE)           // check for the WiFi module:
    {                                          // don't continue
    Serial.println("Communication with WiFi module failed!");
    while (true);
    }
  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") { Serial.println("Please upgrade the firmware");  }
                                               // by default the local IP address of will be 192.168.4.1
                                               // you can override it with the following: WiFi.config(IPAddress(10, 0, 0, 1));
  Serial.print("Creating access point named: ");   // print the network name (SSID);
  Serial.println(ssid);
                                               // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING)               // don't continue
    {
    Serial.println("Creating access point failed");
    while (true);
    } 
  delay(10000);                                // wait 10 seconds for connection:
  server.begin();                              // start the web server on port 80  
  printWiFiStatus();                           // you're connected now, so print out the status
}
//--------------------------------------------
// SERVER Update routine done every second
//--------------------------------------------
void WIFIServercheck(void)     
{
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
 
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED off<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(led, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(led, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }                     
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}
                          #endif
