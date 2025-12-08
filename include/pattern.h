/*
On cue:
+ duplicate non-synced nodes and link to synced nodes
+ store ids in voice object and push to empty voice slot
+ assign voices to relevant cues
+ retirgger event scheduling
  - push to event queue
  - every tick, check queue time and update/dequeue
+ end of lifetime, remove duplicate nodes and free voice slot

Need to implement:
+ voice object and voice builder logic
  - VoiceInstance, VoiceTemplate, VoiceBuilder, VoiceManager
+ global voice array with removal logic and free slot tracking
+ universal event queue
  - note on
    - spawn voice
    - sync reset (for synced nodes)
  - note off - release voice if applicable
  - set param
  - kill all / emergency stop
*/

#pragma once

#include <queue>
#include <string>

#include "event.h"
#include "voice.h"

class PatternEngine {
  std::queue<Event> eventQueue;
  std::unordered_map<std::string, int> cueMap;

public:
  PatternEngine() = default;
  ~PatternEngine() = default;
};
