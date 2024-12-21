#include <ST7789v_arduino.h> // Hardware-specific library for ST7789 (with or without CS pin)
#include <SPI.h>

// define pins for tft & button
#define TFT_DC    7
#define TFT_RST   5 
#define TFT_CS    9
// TFT_MOSI  11
// TFT_SCLK  13
#define BUTTON 2

int counter = 0;
boolean reachedEnd = false;

// angled rectangles
int xL[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int yL[] = {40, 44, 46, 48, 47, 45, 49, 43, 42, 43, 41, 47, 42};
int aL[] = {10, 5, -5, -10, 30, -20, 28, -10, 30, 5, -5, 15, -15};
uint16_t cL[] = {RED, YELLOW, BLUE, MAGENTA, CYAN, RED, WHITE, WHITE, BLUE, GREEN, CYAN, YELLOW, YELLOW};

uint16_t colorHash[] = {GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, CYAN};

ST7789v_arduino tft = ST7789v_arduino(TFT_DC, TFT_RST, TFT_CS); // create tft object

// -------------------------------------------------

void setup() {

  Serial.begin(9600); // Connect to serial moniter

  tft.init(240, 240);   // initialize a ST7789 chip, 240x240 pixels
  tft.fillScreen(BLACK);
  tft.setTextWrap(true);
  tft.setTextSize(5);

  pinMode(BUTTON, INPUT);  // initialize button

  randomSeed(analogRead(A4));

  while(digitalRead(BUTTON) == LOW) {}

  tone(A5, 698, 200);
  delay(150);
  tone(A5, 622, 500);
}

// -------------------------------------------------

void loop() {
  // put your main code here, to run repeatedly

  if((xL[0] + xL[1] + xL[2])/3 - 63 > tft.width() + 10) {
    reachedEnd = true;
  }

  tft.fillRect((xL[0] + xL[1] + xL[2])/3 - 63, (yL[0] + yL[1])/2 - 40, 126, 80, BLACK);
  tft.fillRect(0, 20, 10, 40, YELLOW);
  tft.drawRect(20, 20, 200, 190, WHITE);
  tft.setTextColor(colorHash[(int)(random(0, 7))]);
  tft.setCursor(32, 30);
  tft.setTextSize(6);
  tft.print("HAPPY");
  tft.setCursor(37, 125);
  tft.setTextSize(10);
  tft.print("DAY");
  tft.setCursor(37, 85);
  tft.setTextSize(4);
  tft.print("FATHERS");

  for(int i = 0; i < sizeof(xL)/sizeof(xL[0]); i++) {
    angleFillRect(xL[i], yL[i], aL[i], 10, 20, cL[i]);
  }
  
  for(int i = 0; !reachedEnd && i < sizeof(xL)/sizeof(xL[0]); i++) {
    xL[i] += (int)(random(8, 30 - counter));
    yL[i] += (int)(random(counter, counter + 3));
    aL[i] += (int)(random(-20, 21));
  }

  delay(50);
  counter++;
}

// -------------------------------------------------

void angleFillRect(int x1, int y1, int a, int h, int w, uint16_t color) {

  a %= 360;
  if(a > 180) {
    a -= 360;
  } else if(a < -180) {
    a += 360;
  }

  int tn = a*22;

  if(a == 180) {
    tft.fillRect(x1 - w, y1, w, h, color);
  } else if(a > 90) {
    tn -= 1980;
    angleFillRect(x1 - w*tn/1260, y1 + w - w*tn/3780, a - 90, w, h, color);
  } else if(a > 45) {

    tn = 1980 - tn;

    int bottomPointY = y1 - h*tn/1260;
    int topPointY = y1 + w - w*tn/3780;
    int x2 = x1 + w*tn/1260 + h - h*tn/3780;
    int y2 = bottomPointY + topPointY - y1;

    for(int i = x1; i <= x2; i++) {
      for(int j = bottomPointY; j <= topPointY; j++) {
        if(inBounds(i, j, x1, y1, x2, y2, 1980 - tn, 1260)) {
          tft.drawPixel(i, j, color);
        }
      }
    }   
  } else if(a > 0) {

    int bottomPointY = y1 - h + h*tn/3780;
    int topPointY = y1 + tn*w/1260;
    int x2 = x1 + w - w*tn/3780 + tn*h/1260;
    int y2 = bottomPointY + topPointY - y1;

    for(int i = x1; i <= x2; i++) {
      for(int j = bottomPointY; j <= topPointY; j++) {
        if(inBounds(i, j, x1, y1, x2, y2, tn, 1260)) {
          tft.drawPixel(i, j, color);
        }
      }
    }
  } else if(a > -90) {
    tn *= -1;
    angleFillRect(x1 - h*tn/1260, y1 - h + h*tn/3780, 90 + a, w, h, color);
  } else if(a == -90) {
    tft.fillRect(x1 - h, y1 - w, h, w, color);
  } else {
    tn = 3960 + tn;
    angleFillRect(x1 - w + w*tn/3780 - h*tn/1260, y1 - w*tn/1260 + h - h*tn/3780, 180 + a, h, w, color);
  }
}

// -------------------------------------------------

boolean inBounds(int ptX, int ptY, int x1, int y1, int x2, int y2, int tn, int td) {

  if(tn > 990) {
    tn = 1980 - tn;
    int temp = tn;
    tn = td;
    td = temp;
  }

  if(td*(ptY - y1) <= tn*(ptX - x1) && tn*(ptY - y1) >= -td*(ptX - x1) && tn*(ptY - y2) <= -td*(ptX - x2) && td*(ptY - y2) >= tn*(ptX - x2)) {
    return true;
  }

  return false;
}
