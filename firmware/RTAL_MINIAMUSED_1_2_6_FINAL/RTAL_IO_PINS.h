#pragma once
#include <Arduino.h>
#include "driver/i2s.h"

#define MIDI_RX_PIN      18
#define MIDI_TX_PIN      17

#define I2S_OUTPUT_NUM   I2S_NUM_0
#define I2S_MCLK         I2S_PIN_NO_CHANGE
#define I2S_BCLK         16
#define I2S_LRCK         15
#define I2S_DOUT         7

#define RTAL_SAMPLE_RATE     48000
#define RTAL_DMA_BUF_COUNT       8
#define RTAL_DMA_BUF_LEN       128
