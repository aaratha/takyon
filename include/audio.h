#pragma once

#include "graph.h"
#include "miniaudio.h"

#include <atomic>
#include <vector>

class AudioManager {
  ma_device_config deviceConfig{};
  ma_device device{};
  std::atomic<bool> running{false};
  bool audioInitialized;

  std::vector<unique_ptr<Node>> &nodes;
  std::vector<int> &topoOrder;
  std::vector<int> &sinkedNodes;

  static void dataCallback(ma_device *pDevice, void *pOutput,
                           const void * /*pInput*/, ma_uint32 frameCount);

public:
  AudioManager(Graph &graph);
  ~AudioManager();
};
