#include <TFT_eSPI.h>
#include "gauge1.h"
#include "gauge2.h"
#include "gauge3.h"
#include "gauge4.h"
#include "gauge5.h"
#include "gauge6.h"
#include "font.h"

enum CONSOLE_STATE {
  STATE_ROOT_MENU
};
char cmdBuffer[80];
int ptrBuffer;
int state;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite ln = TFT_eSprite(&tft);

double rad = 0.01745;
int angle;

int sx = 120;
int sy = 120;
int r = 76;

int32_t x[360];
int32_t y[360];
int32_t x2[360];
int32_t y2[360];

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

int chosenOne = 0;
int minValue[6] = { 0,20,0,0,0,80 };
int maxValue[6] = { 40,100,60,80,70,160 };
int dbounce = 0;

void rcvCharacter(uint8_t chr);
void handleConsoleCmd();
void handleShortCmd();
void showHelp();
void handleConfigCmd();

void setup() {
  ptrBuffer = 0;
  state = STATE_ROOT_MENU;
  // ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  // ledcAttachPin(5, pwmLedChannelTFT);
  // ledcWrite(pwmLedChannelTFT, 90);

  pinMode(12, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  img.setSwapBytes(true);
  tft.fillScreen(TFT_ORANGE);
  img.createSprite(240, 240);


  tft.setPivot(60, 60);
  img.setTextDatum(4);
  img.setTextColor(TFT_BLACK, 0xAD55);
  img.setFreeFont(&Orbitron_Medium_28);

  int i = 0;
  int a = 136;

  while (a != 44) {
    x[i] = r * cos(rad * a) + sx;
    y[i] = r * sin(rad * a) + sy;
    x2[i] = (r - 20) * cos(rad * a) + sx;
    y2[i] = (r - 20) * sin(rad * a) + sy;
    i++;
    a++;
    if (a == 360)
      a = 0;
  }
}

//min angle 136 or 137
//max angle 43

int a1, a2;
int result = 0;
int value = 0;
int serialCnt;

void loop() {
  uint8_t in_byte;
  serialCnt = 0;
  while ((Serial.available() > 0) && serialCnt < 128) {
    serialCnt++;
    in_byte = Serial.read();
    rcvCharacter(in_byte);
  }
  result = map(value, 0, 4095, minValue[chosenOne], maxValue[chosenOne]);
  angle = map(result, minValue[chosenOne], maxValue[chosenOne], 0, 267);



  if (chosenOne == 0)
    img.pushImage(0, 0, 240, 240, gauge1);
  if (chosenOne == 1)
    img.pushImage(0, 0, 240, 240, gauge2);
  if (chosenOne == 2)
    img.pushImage(0, 0, 240, 240, gauge3);
  if (chosenOne == 3)
    img.pushImage(0, 0, 240, 240, gauge4);
  if (chosenOne == 4)
    img.pushImage(0, 0, 240, 240, gauge5);
  if (chosenOne == 5)
    img.pushImage(0, 0, 240, 240, gauge6);

  if (chosenOne == 5)
    img.drawFloat(result / 10.00, 2, 120, 114);
  else if (chosenOne == 4)
    img.drawString(String(result * 100), 120, 114);
  else
    img.drawString(String(result), 120, 114);
  //img.drawString(String(analogRead(22)), 30,10,2);

  a1 = angle - 4;
  a2 = angle + 4;

  if (a1 < 0)
    a1 = angle - 4 + 359;
  if (a2 >= 359)
    a2 = angle + 4 - 359;

  if (result <= minValue[chosenOne] + 4)
    img.fillTriangle(x[angle], y[angle], x2[angle], y2[angle], x2[a2 + 2], y2[a2 + 2], TFT_RED);
  else if (result >= maxValue[chosenOne] - 4)
    img.fillTriangle(x[angle], y[angle], x2[a1 - 2], y2[a1 - 2], x2[angle], y2[angle], TFT_RED);
  else
    img.fillTriangle(x[angle], y[angle], x2[a1], y2[a1], x2[a2], y2[a2], TFT_RED);


  img.pushSprite(0, 0);
}

void rcvCharacter(uint8_t chr) {
  if (chr == 10 || chr == 13) { //command done. Parse it.
    handleConsoleCmd();
    ptrBuffer = 0; //reset line counter once the line has been processed
  } else {
    cmdBuffer[ptrBuffer++] = (unsigned char)chr;
    if (ptrBuffer > 79)
      ptrBuffer = 79;
  }
}

void handleConsoleCmd() {
  if (state == STATE_ROOT_MENU) {
    if (ptrBuffer == 1) {
      //command is a single ascii character
      handleShortCmd();
    } else { //at least two bytes
      boolean equalSign = false;
      for (int i = 0; i < ptrBuffer; i++) if (cmdBuffer[i] == '=') equalSign = true;
      cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
      if (equalSign) handleConfigCmd();
    }
    ptrBuffer = 0; //reset line counter once the line has been processed
  }
}

void handleShortCmd() {
  uint8_t val;

  switch (cmdBuffer[0]) {
    //non-lawicel commands
    case 'h':
    case '?':
    case 'H':
    default:
      showHelp();
      break;
  }
}

void handleConfigCmd() {
  int i;
  int newValue;
  char* newString;
  bool writeEEPROM = false;
  bool writeDigEE = false;
  char* dataTok;

  //Logger::debug("Cmd size: %i", ptrBuffer);
  if (ptrBuffer < 6)
    return; //4 digit command, =, value is at least 6 characters
  cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
  String cmdString = String();
  unsigned char whichEntry = '0';
  i = 0;

  while (cmdBuffer[i] != '=' && i < ptrBuffer) {
    cmdString.concat(String(cmdBuffer[i++]));
  }
  i++; //skip the =
  if (i >= ptrBuffer) {
    Serial.printf("Command needs a value..ie TORQ=3000");
    Serial.printf("");
    return; //or, we could use this to display the parameter instead of setting
  }

  // strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
  newValue = strtol((char*)(cmdBuffer + i), NULL, 0); //try to turn the string into a number
  newString = (char*)(cmdBuffer + i); //leave it as a string

  cmdString.toUpperCase();

  if (cmdString.startsWith("VALUE")) {
    if (newValue >= 0 && newValue <= 4095) {
      Serial.printf("Setting CAN0 Nominal Speed to %i", newValue);
      value = newValue;

      writeEEPROM = true;
    } else Serial.printf("Invalid baud rate! Enter a value 32000 - 1000000");
  } else if (cmdString == String("MENU")) {
    if (newValue >= 0 && newValue <= 6) {
      Serial.printf("Setting CAN0 Nominal Speed to %i", newValue);
      chosenOne = newValue;

      writeEEPROM = true;
    } else Serial.printf("Invalid baud rate! Enter a value 1 - 6");
  } else {
    Serial.printf("Unknown command");
  }
}

void showHelp() {
  //Show build # here as well in case people are using the native port and don't get to see the start up messages
  Serial.println("System Menu:");
  Serial.println();
  Serial.println("Enable line endings of some sort (LF, CR, CRLF)");
  Serial.println();
  Serial.println("Short Commands:");
  Serial.println("h = help (displays this message)");
  Serial.println();
  Serial.println("Config Commands (enter command=newvalue). Current values shown in parenthesis:");
  Serial.println();

  Serial.printf("MENU=%i - Escolha entre os menus (0=coolant pressure, 1=coolant temp)", chosenOne);
  Serial.println();
  Serial.printf("VALUE=%i - set log level (0-4095)", value);
  Serial.println();
}
