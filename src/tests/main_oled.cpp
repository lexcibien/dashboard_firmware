#include <Arduino.h>
#include <U8g2lib.h>
#include "bitmap_icons.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  u8g2.begin();
  Serial.begin(115200);
  delay(5000);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Iniciando Exemplo");
}

void loop(void) {
  Serial.println("Texto Hello World");
  digitalWrite(LED_BUILTIN, HIGH);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(0, 30, "Hello World!");
  u8g2.drawXBMP(0, 0, 12, 8, image_battery_100_bits);
  u8g2.drawXBMP(25, 0, 8, 8, epd_bitmap_clock);
  u8g2.drawXBMP(34, 0, 8, 8, epd_bitmap_fruit);
  u8g2.sendBuffer();

  delay(1000);

  Serial.println("Texto Teste limpar");
  digitalWrite(LED_BUILTIN, LOW);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(0, 30, "Teste limpar!");
  u8g2.drawXBMP(0, 0, 8, 8, epd_bitmap_clock);
  u8g2.drawXBMP(9, 0, 8, 8, epd_bitmap_fruit);
  u8g2.sendBuffer();
  delay(1000);
}
