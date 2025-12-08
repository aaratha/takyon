#pragma once

#include <cstdint>

enum EventType {
  NoteOn,   // spawn voice or sync reset
  NoteOff,  // release temp voice
  SetParam, // set node parameter
  KillAll
};

struct NoteOnPayload {
  uint16_t templateId;
  float pitch;
  float velocity;
};

struct NoteOffPayload {
  uint16_t voiceId;
};

struct SetParamPayload {
  uint16_t voiceId;
  uint16_t paramId;
  float value;
};

struct Event {
  EventType type;
  uint64_t tsSamples;
  union {
    NoteOnPayload spawn;
    NoteOffPayload release;
    SetParamPayload setParam;
  };
};
