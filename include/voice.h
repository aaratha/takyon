#pragma once

#include <memory>
#include <queue>
#include <vector>

#include "globals.h"
#include "graph.h"

enum class VoiceState { Active, Releasing, Inactive };

enum class ParamType { Float, Bool, Waveform };

struct NodeSpec {};

struct EdgeSpec {
  int parentId;
  int childId;
};

struct ParamSpec {
  ParamType type;
  int nodeId;
  int paramId;
};

struct ParamBinding {
  ParamType type;
  union {
    std::atomic<float> *f;
    std::atomic<bool> *b;
    std::atomic<Waveform> *w;
  } ptr;
};

// for voice creation and instancing (on cue)
struct VoiceTemplate {
  std::vector<int> nodeIds; // original or to be copied
  std::vector<std::vector<int>> edges;
  std::vector<ParamBinding> paramBindings;
};

// voice object with stored node ids and retrigger logic
struct VoiceInstance {
  int voiceId;
  int templateId;           // original nodes
  std::vector<int> nodeIds; // original or new nodes
  std::vector<std::vector<int>> edges;
  std::vector<ParamBinding> paramBindings;
  VoiceState state;
};

// Stores and manages slots and active voices
class VoiceManager {
  int maxVoices;
  std::queue<int> freeVoiceIds;
  std::vector<std::unique_ptr<VoiceTemplate>> voiceTemplates;
  std::vector<std::unique_ptr<VoiceInstance>> voiceInstances;

  Graph &graph;

public:
  VoiceManager(Graph &graph, int maxVoices);
  ~VoiceManager();

  int registerTemplate(std::unique_ptr<VoiceTemplate> voiceTemplate);
  int allocateVoice(std::unique_ptr<VoiceTemplate> voiceTemplate);
  void freeVoice(int voiceId);
  void freeAllVoices();
};
