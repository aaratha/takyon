#pragma once

#include "graph.h"

#include "globals.h"

struct ControlNode : Node {
  std::vector<std::atomic<float> *> targets;
  void addTarget(std::atomic<float> *target);
};

struct EffectNode : Node {
  std::vector<std::atomic<float> *> inputs;
  void addInput(std::atomic<float> *input);
};

struct Oscillator : Node {
  std::atomic<float> amp{1.0f};
  std::atomic<float> freq{440.0f};
  std::atomic<float> phase{0.0f};
  std::atomic<Waveform> type{Waveform::Sine};

  void update() override;

  static std::unique_ptr<Oscillator> init(float amp_ = 1.0f,
                                          float freq_ = 440.0f,
                                          Waveform type_ = Waveform::Sine);
};

struct LFO : ControlNode {
  std::atomic<float> base{0.0f};
  std::atomic<float> amp{1.0f};
  std::atomic<float> freq{5.0f};
  std::atomic<float> shift{0.0f};
  std::atomic<float> phase{0.0f};
  std::atomic<Waveform> type{Waveform::Sine};

  void update() override;

  static std::unique_ptr<LFO> init(float base_ = 0.0f, float amp_ = 1.0f,
                                   float freq_ = 5.0f, float shift_ = 0.0f,
                                   Waveform type_ = Waveform::Sine);
};

struct Filter : EffectNode {
  std::atomic<float> cutoff{500.0f};
  std::atomic<float> q{1.0f};
  float x1 = 0.0f;
  float x2 = 0.0f;
  float y1 = 0.0f;
  float y2 = 0.0f;

  void update() override;

  static std::unique_ptr<Filter> init(float cutoff_ = 500.0f, float q_ = 1.0f);
};
