#include "voice.h"

VoiceManager::VoiceManager(Graph &graph, int maxVoices)
    : graph(graph), maxVoices(maxVoices) {}

VoiceManager::~VoiceManager() = default;

int VoiceManager::registerTemplate(std::unique_ptr<VoiceTemplate> vt) {
  if (freeVoiceIds.size() > 0) {
    int id = freeVoiceIds.front();
    freeVoiceIds.pop();
    voiceTemplates[id] = std::move(vt);
    return id;
  } else {
    voiceTemplates.push_back(std::move(vt));
    return voiceTemplates.size() - 1;
  }
}
