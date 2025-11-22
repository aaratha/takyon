#include "audio.h"

#include "globals.h"
#include <atomic>

AudioManager::AudioManager(Graph &graph)
    : audioInitialized(false), nodes(graph.getNodes()),
      topoOrder(graph.getTopoOrder()), sinkedNodes(graph.getSinkedNodes()) {

  if (audioInitialized)
    return;

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = DEVICE_FORMAT;
  deviceConfig.playback.channels = DEVICE_CHANNELS;
  deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
  deviceConfig.dataCallback = AudioManager::dataCallback;
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

AudioManager::~AudioManager() {
  if (audioInitialized) {
    ma_device_uninit(&device);
    audioInitialized = false;
    running.store(false);
  }
}

void AudioManager::dataCallback(ma_device *pDevice, void *pOutput,
                                const void * /*pInput*/, ma_uint32 frameCount) {
  auto *manager = static_cast<AudioManager *>(pDevice->pUserData);
  float *out = static_cast<float *>(pOutput);

  for (ma_uint32 i = 0; i < frameCount; i++) {
    for (int i : manager->topoOrder) {
      if (!manager->nodes[i]->sinked.load(std::memory_order_relaxed))
        continue;
      manager->nodes[i]->update();
    }

    // mix
    float sample = 0.0f;
    for (int i : manager->sinkedNodes) {
      sample += manager->nodes[i]->out.load(std::memory_order_relaxed);
    }

    *out++ = sample;
    *out++ = sample;
  }
}
