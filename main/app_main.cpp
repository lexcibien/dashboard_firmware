#include "Arduino.h"
#include "TFT_eSPI.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "font.h"
#include "gauge1.h"
#include "gauge2.h"
#include "gauge3.h"
#include "gauge4.h"
#include "gauge5.h"
#include "gauge6.h"

namespace {

constexpr int kGaugeCount = 6;
constexpr int kNeedlePointCount = 268;
constexpr int kNeedleStartDegrees = 136;
constexpr int kNeedleEndDegrees = 44;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kDegreesToRadians = kPi / 180.0F;
constexpr int kSpriteWidth = 240;
constexpr int kSpriteHeight = 240;
constexpr int kRawMinimum = 0;
constexpr int kRawMaximum = 4095;
constexpr TickType_t kRenderPeriod = pdMS_TO_TICKS(33);

const uint16_t* const kGaugeImages[kGaugeCount] = {
    gauge1,
    gauge2,
    gauge3,
    gauge4,
    gauge5,
    gauge6,
};

constexpr int kMinimumValues[kGaugeCount] = {0, 20, 0, 0, 0, 80};
constexpr int kMaximumValues[kGaugeCount] = {40, 100, 60, 80, 70, 160};

TFT_eSPI tft;
TFT_eSprite sprite(&tft);

int32_t needleX[kNeedlePointCount];
int32_t needleY[kNeedlePointCount];
int32_t needleInnerX[kNeedlePointCount];
int32_t needleInnerY[kNeedlePointCount];

int selectedGauge = 0;
int rawValue = 0;

char commandBuffer[80];
size_t commandLength = 0;
bool commandOverflow = false;

int clampGauge(int gauge) {
    if (gauge < 0) {
        return 0;
    }
    if (gauge >= kGaugeCount) {
        return kGaugeCount - 1;
    }
    return gauge;
}

int wrapNeedleIndex(int index) {
    while (index < 0) {
        index += kNeedlePointCount;
    }
    while (index >= kNeedlePointCount) {
        index -= kNeedlePointCount;
    }
    return index;
}

int mapValue(int input, int inputMinimum, int inputMaximum,
             int outputMinimum, int outputMaximum) {
    if (inputMaximum <= inputMinimum) {
        return outputMinimum;
    }

    const int boundedInput = input < inputMinimum
                                 ? inputMinimum
                                 : (input > inputMaximum ? inputMaximum : input);
    const long inputRange = inputMaximum - inputMinimum;
    const long outputRange = outputMaximum - outputMinimum;
    return outputMinimum + static_cast<int>(
                               ((boundedInput - inputMinimum) * outputRange) /
                               inputRange);
}

void initializeNeedleGeometry() {
    int point = 0;
    for (int degrees = kNeedleStartDegrees;
         degrees != kNeedleEndDegrees && point < kNeedlePointCount;
         degrees = (degrees + 1) % 360) {
        const float radians = degrees * kDegreesToRadians;
        needleX[point] = static_cast<int32_t>(76.0F * std::cos(radians) + 120.0F);
        needleY[point] = static_cast<int32_t>(76.0F * std::sin(radians) + 120.0F);
        needleInnerX[point] =
            static_cast<int32_t>(56.0F * std::cos(radians) + 120.0F);
        needleInnerY[point] =
            static_cast<int32_t>(56.0F * std::sin(radians) + 120.0F);
        ++point;
    }
}

void printHelp() {
    Serial.println("Dashboard TFT development console");
    Serial.println("VALUE=0..4095 - set the raw gauge value");
    Serial.println("MENU=0..5 - select the gauge");
}

bool parseCommandValue(const char* text, int& value) {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed < static_cast<long>(INT32_MIN) ||
        parsed > static_cast<long>(INT32_MAX)) {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

void handleCommandLine() {
    if (commandOverflow) {
        Serial.println("ERR command too long");
        commandLength = 0;
        commandOverflow = false;
        return;
    }

    commandBuffer[commandLength] = '\0';
    if (commandLength == 0) {
        return;
    }

    if (commandLength == 1 &&
        (commandBuffer[0] == 'h' || commandBuffer[0] == 'H' ||
         commandBuffer[0] == '?')) {
        printHelp();
        return;
    }

    char* equals = std::strchr(commandBuffer, '=');
    if (equals == nullptr) {
        Serial.println("ERR expected COMMAND=value");
        return;
    }

    *equals = '\0';
    for (char* command = commandBuffer; *command != '\0'; ++command) {
        if (*command >= 'a' && *command <= 'z') {
            *command = static_cast<char>(*command - ('a' - 'A'));
        }
    }

    int parsedValue = 0;
    if (!parseCommandValue(equals + 1, parsedValue)) {
        Serial.println("ERR invalid integer");
        return;
    }

    if (std::strcmp(commandBuffer, "VALUE") == 0) {
        if (parsedValue < kRawMinimum || parsedValue > kRawMaximum) {
            Serial.println("ERR VALUE must be between 0 and 4095");
            return;
        }
        rawValue = parsedValue;
        Serial.printf("OK VALUE=%d\n", rawValue);
        return;
    }

    if (std::strcmp(commandBuffer, "MENU") == 0) {
        if (parsedValue < 0 || parsedValue >= kGaugeCount) {
            Serial.println("ERR MENU must be between 0 and 5");
            return;
        }
        selectedGauge = parsedValue;
        Serial.printf("OK MENU=%d\n", selectedGauge);
        return;
    }

    Serial.println("ERR unknown command");
}

void receiveConsoleInput() {
    while (Serial.available() > 0) {
        const int received = Serial.read();
        if (received < 0) {
            return;
        }

        const char character = static_cast<char>(received);
        if (character == '\n' || character == '\r') {
            handleCommandLine();
            commandLength = 0;
            commandOverflow = false;
            continue;
        }

        if (commandLength < sizeof(commandBuffer) - 1U) {
            commandBuffer[commandLength++] = character;
        } else {
            commandOverflow = true;
        }
    }
}

void renderGauge() {
    const int gauge = clampGauge(selectedGauge);
    const int displayValue = mapValue(rawValue, kRawMinimum, kRawMaximum,
                                      kMinimumValues[gauge], kMaximumValues[gauge]);
    const int angle = mapValue(displayValue, kMinimumValues[gauge],
                               kMaximumValues[gauge], 0,
                               kNeedlePointCount - 1);
    const int previousAngle = wrapNeedleIndex(angle - 4);
    const int nextAngle = wrapNeedleIndex(angle + 4);

    sprite.pushImage(0, 0, kSpriteWidth, kSpriteHeight, kGaugeImages[gauge]);

    char valueText[20];
    if (gauge == 5) {
        const float decimalValue = static_cast<float>(displayValue) / 10.0F;
        std::snprintf(valueText, sizeof(valueText), "%.2f", decimalValue);
    } else if (gauge == 4) {
        std::snprintf(valueText, sizeof(valueText), "%d", displayValue * 100);
    } else {
        std::snprintf(valueText, sizeof(valueText), "%d", displayValue);
    }
    sprite.drawString(valueText, 120, 114);

    if (displayValue <= kMinimumValues[gauge] + 4) {
        sprite.fillTriangle(needleX[angle], needleY[angle],
                           needleInnerX[angle], needleInnerY[angle],
                           needleInnerX[wrapNeedleIndex(nextAngle + 2)],
                           needleInnerY[wrapNeedleIndex(nextAngle + 2)], TFT_RED);
    } else if (displayValue >= kMaximumValues[gauge] - 4) {
        sprite.fillTriangle(needleX[angle], needleY[angle],
                           needleInnerX[wrapNeedleIndex(previousAngle - 2)],
                           needleInnerY[wrapNeedleIndex(previousAngle - 2)],
                           needleInnerX[angle], needleInnerY[angle], TFT_RED);
    } else {
        sprite.fillTriangle(needleX[angle], needleY[angle],
                           needleInnerX[previousAngle], needleInnerY[previousAngle],
                           needleInnerX[nextAngle], needleInnerY[nextAngle], TFT_RED);
    }

    sprite.pushSprite(0, 0);
}

}  // namespace

extern "C" void app_main() {
    initArduino();
    Serial.begin(115200);

    initializeNeedleGeometry();

    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);

    if (sprite.createSprite(kSpriteWidth, kSpriteHeight) == nullptr) {
        Serial.println("FATAL: unable to allocate TFT sprite");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    sprite.setSwapBytes(true);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_BLACK, 0xAD55);
    sprite.setFreeFont(&Orbitron_Medium_28);

    Serial.println("Dashboard TFT ESP-IDF demo ready");
    printHelp();

    while (true) {
        receiveConsoleInput();
        renderGauge();
        vTaskDelay(kRenderPeriod);
    }
}
