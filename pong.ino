#include <ST7789v_arduino.h> // Hardware-specific library for ST7789 (with or without CS pin)
#include <SPI.h>

// define pins for tft
#define TFT_DC    7
#define TFT_RST   6 
#define TFT_CS    10
#define TFT_MOSI  11   // don't touch
#define TFT_SCLK  13   // don't touch
#define TFT_BL    5

// define pin for button
#define bHP 3 // button high pin
#define bLP 4 // button low pin
#define BUTTON 2

ST7789v_arduino tft = ST7789v_arduino(TFT_DC, TFT_RST, TFT_CS); // create tft object

// object variables
int playerPaddleX, playerPaddleY, paddleHeight = 20, paddleWidth = 5, computerPaddleX = 230, computerPaddleY = 120, ballX = 120, ballY = 120, ballHeight = 5, ballWidth = 5;

// speed variables
int ballSpeedX = -7, ballSpeedY = -2, playerPaddleSpeed = 6, computerPaddleSpeed = 9;

// score variables
int scorePlayer = 0, scoreComputer = 0, winningScore = 3;

// frame time variable
int currentTime = 0, lastFrame = 0;

/* ------------------------------------------------------------------------------------------ */

// initialize stuff
void setup(void) {
  Serial.begin(9600); // Connect to serial moniter

  tft.init(240, 240);   // initialize a ST7789 chip, 240x240 pixels
  tft.fillRect(0, 0, 240, 320, BLACK);

  pinMode(BUTTON, INPUT);  // initialize button

  pinMode(bHP, OUTPUT);
  digitalWrite(bHP, HIGH); // give button power
  pinMode(bLP, OUTPUT);
  digitalWrite(bLP, LOW);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // initialize backlight

  screen("begin"); // start the game
}

/* ------------------------------------------------------------------------------------------ */

// screens for before, during and after
void screen(String buttonText) {
  tft.fillScreen(BLACK);

  // display button
  tft.drawRoundRect(120 - 19*buttonText.length()/2, 120, 19*buttonText.length(), 30, 5, WHITE);
  tft.setTextColor(WHITE);
  tft.setTextWrap(true);
  tft.setTextSize(3);
  tft.setCursor(120 - 19*buttonText.length()/2 + 5, 125);
  tft.print(buttonText);
  
  // display scores
  tft.setCursor(5, 220);
  tft.setTextSize(3);
  tft.print(scorePlayer);
  tft.setCursor(200, 220);
  tft.print(scoreComputer);

  // wait for button press
  while(true) {
    if(digitalRead(BUTTON) == HIGH) {
      break;
    }
  }

  // begin game
  reset();
}

/* ------------------------------------------------------------------------------------------ */

// resetScores
void fullReset() {
  scorePlayer = 0;
  scoreComputer = 0;

  reset();
}

/* ------------------------------------------------------------------------------------------ */

// reset everything to default values
void reset() {
  tft.fillScreen(BLACK);

  // reset positions
  playerPaddleX = 5;
  playerPaddleY = 120;

  computerPaddleX = 230;
  computerPaddleY = 120;

  ballX = 118;
  ballY = 118;

  // set starting speed
  ballSpeedX = (int)(random(5, 9));
  if((int)(random(0, 2)) == 0) {
    ballSpeedX *= -1;
  }
  ballSpeedY = (int)(random(-2, 3));

  // draw everything
  tft.drawFastVLine(tft.width()/2, 0, tft.height() - 20, WHITE);
  tft.drawRect(playerPaddleX, playerPaddleY, paddleWidth, paddleHeight, WHITE);
  tft.drawRect(computerPaddleX, computerPaddleY, paddleWidth, paddleHeight, WHITE);
  tft.drawRect(ballX, ballY, ballWidth, ballHeight, WHITE);
  tft.setCursor(5, 225);
  tft.setTextSize(2);
  tft.print(scorePlayer);
  tft.setCursor(215, 225);
  tft.print(scoreComputer);
}

/* ------------------------------------------------------------------------------------------ */

void loop() {

  // move ball
  tft.drawRect(ballX, ballY, ballWidth, ballHeight, BLACK);
  ballX += ballSpeedX;
  ballY += ballSpeedY;
  tft.drawRect(ballX, ballY, ballWidth, ballHeight, WHITE);

  // move player paddle, check for clipping
  if(digitalRead(BUTTON) == HIGH && playerPaddleY < (tft.height() - paddleHeight - 20)) {
    tft.drawRect(playerPaddleX, playerPaddleY, paddleWidth, paddleHeight, BLACK);
    playerPaddleY += playerPaddleSpeed;
    tft.drawRect(playerPaddleX, playerPaddleY, paddleWidth, paddleHeight, WHITE);
  } else if(digitalRead(BUTTON) == LOW && playerPaddleY > 0) {
    tft.drawRect(playerPaddleX, playerPaddleY, paddleWidth, paddleHeight, BLACK);
    playerPaddleY -= playerPaddleSpeed;
    tft.drawRect(playerPaddleX, playerPaddleY, paddleWidth, paddleHeight, WHITE);
  }

  // move computer paddle, check for clipping
  if(computerPaddleY + paddleHeight/2 > ballY && (int)random(0, 7) < 5) {
    tft.drawRect(computerPaddleX, computerPaddleY, paddleWidth, paddleHeight, BLACK);
    computerPaddleY -= computerPaddleSpeed;
    tft.drawRect(computerPaddleX, computerPaddleY, paddleWidth, paddleHeight, WHITE);
  } else if(computerPaddleY + paddleHeight/2 < ballY && (int)random(0, 7) < 5) {
    tft.drawRect(computerPaddleX, computerPaddleY, paddleWidth, paddleHeight, BLACK);
    computerPaddleY += computerPaddleSpeed;
    tft.drawRect(computerPaddleX, computerPaddleY, paddleWidth, paddleHeight, WHITE);
  }

  // check if computer hit ball
  if(ballX + ballWidth > computerPaddleX && (ballY > computerPaddleY + paddleHeight || ballY < computerPaddleY - ballHeight)) {
    scorePlayer++;
    if(scorePlayer < winningScore) {
      screen("next round");
    } else {
      screen("you win");
      fullReset();
    }
  } else if(ballX + ballWidth > computerPaddleX) {
    ballSpeedX *= -1;
    if(ballSpeedY > 0) {
      ballSpeedY += (int)(random(0, 3));
    } else {
      ballSpeedY -= (int)(random(0, 3));
    }
  }

  // check if player hit ball
  if(ballX < playerPaddleX + paddleWidth && (ballY > playerPaddleY + paddleHeight || ballY < playerPaddleY - ballHeight)) {
    scoreComputer++;
    if(scoreComputer < winningScore) {
      screen("next round");
    } else {
      screen("you lose");
      fullReset();
    }
  } else if(ballX < playerPaddleX + paddleWidth) {
    ballSpeedX *= -1;
    if(ballSpeedY > 0) {
      ballSpeedY += (int)(random(0, 3));
    } else {
      ballSpeedY -= (int)(random(0, 3));
    }
  }

  // check if ball hit wall
  if(ballY > 220 - ballHeight || ballY < 0) {
    ballSpeedY *= -1;
    if(ballSpeedX > 0) {
      ballSpeedX += (int)(random(3, 6));
    } else {
      ballSpeedX -= (int)(random(3, 6));
    }
  }
  
  tft.drawFastVLine(tft.width()/2, 0, tft.height() - 20, WHITE);

  // wait for next frame
  while(true) {
    currentTime = millis();
    if(currentTime - lastFrame > 200) {
      break;
    }
  }
  lastFrame = currentTime;
}
