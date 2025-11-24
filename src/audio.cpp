#include "audio.h"

#include "globals.h"
#include <atomic>

AudioEngine::AudioEngine(Graph &graph)
    : audioInitialized(false), nodes(graph.getNodes()),
      topoOrder(graph.getTopoOrder()), sinkedNodes(graph.getSinkedNodes()) {

  if (audioInitialized)
    return;

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = DEVICE_FORMAT;
  deviceConfig.playback.channels = DEVICE_CHANNELS;
  deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
  deviceConfig.dataCallback = AudioEngine::dataCallback;
  deviceConfig.pUserData = this;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    return;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    ma_device_uninit(&device);
    return;
  }

  audioInitialized = true;
  running.store(true);
}

AudioEngine::~AudioEngine() {
  if (audioInitialized) {
    ma_device_uninit(&device);
    audioInitialized = false;
    running.store(false);
  }
}

void AudioEngine::dataCallback(ma_device *pDevice, void *pOutput,
                               const void * /*pInput*/, ma_uint32 frameCount) {
  auto *manager = static_cast<AudioEngine *>(pDevice->pUserData);
  float *out = static_cast<float *>(pOutput);

  for (ma_uint32 frame = 0; frame < frameCount; frame++) {
    for (int nodeId : manager->topoOrder) {
      if (nodeId < 0 || nodeId >= static_cast<int>(manager->nodes.size()))
        continue;
      auto *node = manager->nodes[nodeId].get();
      if (!node)
        continue;
      node->update();
    }

    float sample = 0.0f;
    for (int nodeId : manager->sinkedNodes) {
      if (nodeId < 0 || nodeId >= static_cast<int>(manager->nodes.size()))
        continue;
      auto *node = manager->nodes[nodeId].get();
      if (!node)
        continue;
      sample += node->out.load(std::memory_order_relaxed);
    }

    *out++ = sample;
    *out++ = sample;
  }
}
