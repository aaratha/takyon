#pragma once

#define DEVICE_SAMPLE_RATE 44800.0f
#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2

enum class Waveform : int {
  Sine = 0,
  Saw = 1,
  InvSaw = 2,
  Square = 3,
  Triangle = 4
};
