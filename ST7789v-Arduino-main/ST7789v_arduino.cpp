/***************************************************
  This is a library for the ST7789 IPS SPI display.

  Originally written by Limor Fried/Ladyada for 
  Adafruit Industries.

  Modified by Ananev Ilia
  Modified by Kamran Gasimov
 ****************************************************/

#include "ST7789v_arduino.h"
#include <limits.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include "SPI.h"
// #include <PI.h >

static const uint8_t PROGMEM
  cmd_240x240[] = {                     // Initialization commands for 7789 screens
    10,                               // 9 commands in list:
    ST7789_SWRESET,   ST_CMD_DELAY,     // 1: Software reset, no args, w/delay
      150,                            // 150 ms delay
    ST7789_SLPOUT ,   ST_CMD_DELAY,     // 2: Out of sleep mode, no args, w/delay
      255,                            // 255 = 500 ms delay
    ST7789_COLMOD , 1+ST_CMD_DELAY,     // 3: Set color mode, 1 arg + delay:
      0x55,                           // 16-bit color
      10,                             // 10 ms delay
    ST7789_MADCTL , 1,            // 4: Memory access ctrl (directions), 1 arg:
      0x00,                           // Row addr/col addr, bottom to top refresh
    ST7789_CASET  , 4,            // 5: Column addr set, 4 args, no delay:
      0x00, ST7789_240x240_XSTART,          // XSTART = 0
    (ST7789_TFTWIDTH+ST7789_240x240_XSTART) >> 8,
    (ST7789_TFTWIDTH+ST7789_240x240_XSTART) & 0xFF,   // XEND = 240
    ST7789_RASET  , 4,            // 6: Row addr set, 4 args, no delay:
      0x00, ST7789_240x240_YSTART,          // YSTART = 0
      (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) >> 8,
    (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) & 0xFF,  // YEND = 320
    ST7789_INVON ,   ST_CMD_DELAY,      // 7: Inversion ON
      10,
    ST7789_NORON  ,   ST_CMD_DELAY,     // 8: Normal display on, no args, w/delay
      10,                             // 10 ms delay
    ST7789_DISPON ,   ST_CMD_DELAY,     // 9: Main screen turn on, no args, w/delay
    255 };                          // 255 = 500 ms delay

inline uint16_t swapcolor(uint16_t x) { 
  return (x << 11) | (x & 0x07E0) | (x >> 11);
}

// #if defined (SPI_HAS_TRANSACTION)
  static SPISettings mySPISettings;
  // mySPISettings.setClockDivider(SPI_CLOCK_DIV128);
// #elif defined (__AVR__) || defined(CORE_TEENSY) 
//   static uint8_t SPCRbackup;
//   static uint8_t mySPCR;
// #endif


#if defined (SPI_HAS_TRANSACTION)
#define SPI_BEGIN_TRANSACTION()    if (_hwSPI)    SPI.begin();
#define SPI_END_TRANSACTION()      if (_hwSPI)    SPI.endTransaction()
#else
#define SPI_BEGIN_TRANSACTION()    if (_hwSPI)    SPI.begin();
#define SPI_END_TRANSACTION()      if (_hwSPI)    SPI.endTransaction()
#endif

// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
ST7789v_arduino::ST7789v_arduino(int8_t dc, int8_t rst, int8_t cs) 
  : Adafruit_GFX(ST7789_TFTWIDTH, ST7789_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  _hwSPI = true;
  _SPI9bit = false;
  _sid  = _sclk = -1;
}

// ------------------------------------------------

inline void ST7789v_arduino::spiwrite(uint8_t c) 
{

  #if defined (SPI_HAS_TRANSACTION)
    SPI.transfer(c);
  #elif defined (__AVR__) || defined(CORE_TEENSY)
    SPCRbackup = SPCR;
    SPCR = mySPCR;
    SPI.transfer(c);
    SPCR = SPCRbackup;
  #elif defined (__arm__)
    SPI.setClockDivider(21); //4MHz
    SPI.setDataMode(SPI_MODE2);
    SPI.transfer(c);
  #endif
}

// ------------------------------------------------

void ST7789v_arduino::writecommand(uint8_t c) {

  DC_LOW();
  CS_LOW();
  SPI_BEGIN_TRANSACTION();

  spiwrite(c);

  CS_HIGH();
  SPI_END_TRANSACTION();
}

// ------------------------------------------------

void ST7789v_arduino::writedata(uint8_t c) {
  SPI_BEGIN_TRANSACTION();
  DC_HIGH();
  CS_LOW();
    
  spiwrite(c);

  CS_HIGH();
  SPI_END_TRANSACTION();
}

// ------------------------------------------------

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void ST7789v_arduino::displayInit(const uint8_t *addr) {

  uint8_t  numCommands, numArgs;
  uint16_t ms;
  //<-----------------------------------------------------------------------------------------
  DC_HIGH();
  #if defined(USE_FAST_IO)
      *clkport |=  clkpinmask;
  #else
      digitalWrite(_sclk, HIGH);
  #endif
  //<-----------------------------------------------------------------------------------------

  numCommands = pgm_read_byte(addr++);   // Number of commands to follow
  while(numCommands--) {                 // For each command...
    writecommand(pgm_read_byte(addr++)); //   Read, issue command
    numArgs  = pgm_read_byte(addr++);    //   Number of args to follow
    ms       = numArgs & ST_CMD_DELAY;   //   If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;            //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      writedata(pgm_read_byte(addr++));  //     Read, issue argument
    }

    if(ms) {
      ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      delay(ms);
    }
  }
}

// ------------------------------------------------

// Initialization code common to all ST7789 displays
void ST7789v_arduino::commonInit(const uint8_t *cmdList) {
  _ystart = _xstart = 0;
  _colstart  = _rowstart = 0; // May be overridden in init func

  pinMode(_dc, OUTPUT);
  if(_cs) {
    pinMode(_cs, OUTPUT);
  }

#if defined(USE_FAST_IO)
  dcport    = portOutputRegister(digitalPinToPort(_dc));
  dcpinmask = digitalPinToBitMask(_dc);
  if(_cs) {
  csport    = portOutputRegister(digitalPinToPort(_cs));
  cspinmask = digitalPinToBitMask(_cs);
  }
  
#endif
    
// #if defined (SPI_HAS_TRANSACTION)
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  mySPISettings = SPISettings(0, MSBFIRST, SPI_MODE2);

  // toggle RST low to reset; CS low so it'll listen to us
  CS_LOW();
  if (_rst != -1) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(50);
    digitalWrite(_rst, LOW);
    delay(50);
    digitalWrite(_rst, HIGH);
    delay(50);
  }

  if(cmdList) 
    displayInit(cmdList);
}

// ------------------------------------------------

void ST7789v_arduino::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1,
 uint16_t y1) {

  uint16_t x_start = x0 + _xstart, x_end = x1 + _xstart;
  uint16_t y_start = y0 + _ystart, y_end = y1 + _ystart;
  

  writecommand(ST7789_CASET); // Column addr set
  writedata(x_start >> 8);
  writedata(x_start & 0xFF);     // XSTART 
  writedata(x_end >> 8);
  writedata(x_end & 0xFF);     // XEND

  writecommand(ST7789_RASET); // Row addr set
  writedata(y_start >> 8);
  writedata(y_start & 0xFF);     // YSTART
  writedata(y_end >> 8);
  writedata(y_end & 0xFF);     // YEND

  writecommand(ST7789_RAMWR); // write to RAM
}

// ------------------------------------------------

void ST7789v_arduino::drawPixel(int16_t x, int16_t y, uint16_t color) {

  if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

  setAddrWindow(x,y,x+1,y+1);

  SPI_BEGIN_TRANSACTION();
  DC_HIGH();
  CS_LOW();

  spiwrite(color >> 8);
  spiwrite(color);

  CS_HIGH();
  SPI_END_TRANSACTION();
}

// ------------------------------------------------

void ST7789v_arduino::drawFastVLine(int16_t x, int16_t y, int16_t h,
 uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((y+h-1) >= _height) h = _height-y;
  setAddrWindow(x, y, x, y+h-1);

  uint8_t hi = color >> 8, lo = color;
    
  SPI_BEGIN_TRANSACTION();
  DC_HIGH();
  CS_LOW();

  while (h--) {
    spiwrite(hi);
    spiwrite(lo);
  }

  CS_HIGH();
  SPI_END_TRANSACTION();
}

// ------------------------------------------------

void ST7789v_arduino::drawFastHLine(int16_t x, int16_t y, int16_t w,
  uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((x+w-1) >= _width)  w = _width-x;
  setAddrWindow(x, y, x+w-1, y);

  uint8_t hi = color >> 8, lo = color;

  SPI_BEGIN_TRANSACTION();
  DC_HIGH();
  CS_LOW();

  while (w--) {
    spiwrite(hi);
    spiwrite(lo);
  }

  CS_HIGH();
  SPI_END_TRANSACTION();
}

// ------------------------------------------------

void ST7789v_arduino::fillScreen(uint16_t color) {
  fillRect(0, 0,  _width, _height, color);
}

// ------------------------------------------------

void ST7789v_arduino::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
  uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if(x>=_width || y>=_height || w<=0 || h<=0) return;
  if(x+w-1>=_width)  w=_width -x;
  if(y+h-1>=_height) h=_height-y;
  setAddrWindow(x, y, x+w-1, y+h-1);

  uint8_t hi = color >> 8, lo = color;
    
  SPI_BEGIN_TRANSACTION();

  DC_HIGH();
  CS_LOW();
  
  uint32_t num = (uint32_t)w*h;
  uint16_t num16 = num>>4;
  while(num16--) {
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
    spiwrite(hi); spiwrite(lo);
  }
  uint8_t num8 = num & 0xf;
  while(num8--) { spiwrite(hi); spiwrite(lo); }


  CS_HIGH();
  SPI_END_TRANSACTION();
}

// ------------------------------------------------

void ST7789v_arduino::invertDisplay(boolean i) {
  writecommand(i ? ST7789_INVON : ST7789_INVOFF);
}

/******** low level bit twiddling **********/

// ------------------------------------------------

inline void ST7789v_arduino::CS_HIGH(void) {
  if(_cs) {
    #if defined(USE_FAST_IO)
      *csport |= cspinmask;
    #else
    digitalWrite(_cs, HIGH);
    #endif
  }
}

// ------------------------------------------------

inline void ST7789v_arduino::CS_LOW(void) {
  if(_cs) {
    #if defined(USE_FAST_IO)
    *csport &= ~cspinmask;
    #else
    digitalWrite(_cs, LOW);
    #endif
  }
}

// ------------------------------------------------

inline void ST7789v_arduino::DC_HIGH(void) {
  _DCbit = true;
#if defined(USE_FAST_IO)
  *dcport |= dcpinmask;
#else
  digitalWrite(_dc, HIGH);
#endif
}

// ------------------------------------------------

inline void ST7789v_arduino::DC_LOW(void) {
  _DCbit = false;
#if defined(USE_FAST_IO)
  *dcport &= ~dcpinmask;
#else
  digitalWrite(_dc, LOW);
#endif
}

// ------------------------------------------------

void ST7789v_arduino::init(uint16_t width, uint16_t height) {
  commonInit(NULL);

  _colstart = ST7789_240x240_XSTART;
  _rowstart = ST7789_240x240_YSTART;
  _height = 240;
  _width = 240;

  displayInit(cmd_240x240);

  writedata(ST7789_MADCTL_RGB);

  _xstart = _colstart;
  _ystart = _rowstart;
  // break;
}

// ----------------------------------------------------------

// 0 - off
// 1 - idle
// 2 - normal
// 4 - display off

void ST7789v_arduino::powerSave(uint8_t mode) 
{
  if(mode==0) {
    writecommand(ST7789_POWSAVE);
    writedata(0xec|3);
    writecommand(ST7789_DLPOFFSAVE);
    writedata(0xff);
    return;
  }
  int is = (mode&1) ? 0 : 1;
  int ns = (mode&2) ? 0 : 2;
  writecommand(ST7789_POWSAVE);
  writedata(0xec|ns|is);
  if(mode&4) {
    writecommand(ST7789_DLPOFFSAVE);
    writedata(0xfe);
  }
}

// ------------------------------------------------

void ST7789v_arduino::startWrite(void){

  SPI_BEGIN_TRANSACTION();
  CS_LOW();
}

// ------------------------------------------------

void ST7789v_arduino::endWrite(void){
  CS_HIGH();
  SPI_END_TRANSACTION();
}
