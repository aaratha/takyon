#pragma once

#include <atomic>
#include <memory>
#include <queue>
#include <vector>

#include "globals.h"
#include "graph.h"

enum class VoiceState { Active, Releasing, Inactive };

enum class ParamKind {
  // Oscilator params
  OscFreq,
  OscAmp,
  OscWaveform,
  // LFO params
  LfoBase,
  LfoAmp,
  LfoFreq,
  LfoShift,
  LfoWaveform,
  //
  FilterCutoff,
  FilterQ
};

/* Upon NodeSpec creation:
NodeSpec oscSpec{SyncMode::PerVoice,
                 [] { return Oscillator::init(ampInit, freqInit, typeInit); }};
NodeSpec lfoSpec{SyncMode::Shared,
                 [] { return LFO::init(base, amp, freq, shift, type); }};
NodeSpec filtSpec {SyncMode::PerVoice,
                 [] { return Filter::init(cutoffInit, qInit); }};
SyncMode is also included in factory
*/
using NodeFactory = std::function<std::unique_ptr<Node>()>;

struct NodeSpec {
  SyncMode syncMode{SyncMode::PerVoice}; // if Shared, create once or look up id
  NodeFactory factory;
};

struct EdgeSpec {
  int parentIdx; // template-local indices
  int childIdx;
};

struct ParamSpec {
  ParamKind kind{ParamKind::OscFreq};
  int nodeIdx{-1}; // template-local index
  int paramId{-1}; // dense 0..N-1 per template
};

struct ParamBinding {
  ParamKind kind{ParamKind::OscFreq};
  union {
    std::atomic<float> *f;
    std::atomic<bool> *b;
    std::atomic<Waveform> *w;
  } ptr;
};

// Blueprint for creating per-voice instances.
class VoiceTemplate {
  std::vector<NodeSpec> nodes_;
  std::vector<EdgeSpec> edges_;
  std::vector<ParamSpec> params_;

public:
  VoiceTemplate(std::vector<NodeSpec> nodes, std::vector<EdgeSpec> edges,
                std::vector<ParamSpec> params);
  ~VoiceTemplate() = default;

  const std::vector<NodeSpec> &nodes() const { return nodes_; }
  const std::vector<EdgeSpec> &edges() const { return edges_; }
  const std::vector<ParamSpec> &params() const { return params_; }
};

// voice object with stored node ids and retrigger logic
class VoiceInstance {
  int voiceId{-1};
  int templateId{-1};
  std::vector<int> nodeIds; // graph node IDs owned by this voice
  std::vector<ParamBinding> paramBindings;
  VoiceState state{VoiceState::Inactive};

public:
  VoiceInstance() = default;
  ~VoiceInstance() = default;

  void setIds(int vId, int tplId) {
    voiceId = vId;
    templateId = tplId;
  }

  void setBindings(std::vector<ParamBinding> bindings) {
    paramBindings = std::move(bindings);
  }

  void setNodeIds(std::vector<int> ids) { nodeIds = std::move(ids); }

  void setState(VoiceState s) { state = s; }

  std::vector<int> &getNodeIds() { return nodeIds; }

  VoiceState getState() const { return state; }
};

// Stores and manages slots and active voices
class VoiceManager {
  int maxVoices;
  std::queue<int> freeVoiceIds;
  std::vector<std::unique_ptr<VoiceTemplate>> voiceTemplates;
  std::vector<std::unique_ptr<VoiceInstance>> voiceInstances;
  std::vector<std::vector<int>> sharedNodeIds;

  Graph &graph;

public:
  VoiceManager(Graph &graph, int maxVoices);
  ~VoiceManager();

  int registerTemplate(std::unique_ptr<VoiceTemplate> voiceTemplate);
  std::vector<int> instantiateNodes(int templateId);
  std::vector<ParamBinding> instantiateParams(int templateId);
  int allocateVoice(int templateId);
  void freeVoice(int voiceId);
  void freeAllVoices();
};
