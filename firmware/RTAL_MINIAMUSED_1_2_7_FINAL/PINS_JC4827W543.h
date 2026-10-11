#pragma once
#include <Arduino.h>

// JC4827W543 / NV3041A QSPI
#define LCD_WIDTH       480
#define LCD_HEIGHT      272

#define GFX_CS           45
#define GFX_SCK          47
#define GFX_D0           21
#define GFX_D1           48
#define GFX_D2           40
#define GFX_D3           39
#define GFX_BL            1
#define GFX_SPEED  32000000UL

// GT911
#define TOUCH_SDA         8
#define TOUCH_SCL         4
#define TOUCH_INT         3
#define TOUCH_RES        38

// SD - proven v0.5.4c configuration
#define SD_SCK           12
#define SD_MISO          13
#define SD_MOSI          11
#define SD_CS            10
#define SD_SPEED    8000000UL
