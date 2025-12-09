#include "voice.h"
#include <atomic>

VoiceTemplate::VoiceTemplate(std::vector<NodeSpec> nodes,
                             std::vector<EdgeSpec> edges,
                             std::vector<ParamSpec> params)
    : nodes_(std::move(nodes)), edges_(std::move(edges)),
      params_(std::move(params)) {}

VoiceManager::VoiceManager(Graph &graph, int maxVoices)
    : graph(graph), maxVoices(maxVoices) {
  for (int i = 0; i < maxVoices; ++i) {
    freeVoiceIds.push(i);
  }
  voiceInstances.resize(maxVoices);
}

VoiceManager::~VoiceManager() = default;

int VoiceManager::registerTemplate(std::unique_ptr<VoiceTemplate> vt) {
  int id = static_cast<int>(voiceTemplates.size());
  voiceTemplates.push_back(std::move(vt));
  // no shared nodes yet, all values -1
  std::vector<int> sharedNodeVec(voiceTemplates.back()->nodes().size(), -1);
  sharedNodeIds.push_back(std::move(sharedNodeVec));
  return id;
}

std::vector<int> VoiceManager::instantiateNodes(int templateId) {
  std::vector<int> nodeIds;
  std::unique_ptr<VoiceTemplate> &vt = voiceTemplates[templateId];
  for (int i = 0; i < vt->nodes().size(); i++) {
    const NodeSpec &ns = vt->nodes()[i];
    int id;
    if (ns.syncMode == SyncMode::PerVoice) {

      // Create node and store id
      id = graph.addNode(ns.factory());

    } else if (ns.syncMode == SyncMode::Shared) {

      // check if shared node already exists
      int sharedNodeId = sharedNodeIds[templateId][i];
      if (sharedNodeId == -1) {

        // Create new shared node and store id
        id = graph.addNode(ns.factory());
        sharedNodeIds[templateId][i] = id;

      } else {

        // store shared node id
        id = sharedNodeId;
      }
    }
    nodeIds.push_back(id);
  }

  // add edges
  for (int i = 0; i < vt->edges().size(); i++) {
    const EdgeSpec &es = vt->edges()[i];
    int parentId = nodeIds[es.parentIdx];
    int childId = nodeIds[es.childIdx];
    graph.addEdge(parentId, childId);
  }
  graph.sort();

  return nodeIds;
}

std::vector<ParamBinding> VoiceManager::instantiateParams(int templateId) {
  // stores param kind for checking and pointer to relevant atomic
  std::vector<ParamBinding> bindings;

  // IF param edit is within voice instance,  simply count nodes and
  // look up param counts in table (e.g. osc = 3), adding them
  // until reaching desired param id. perhaps do ParamKind : int
  // then look up desired atomic in bindings table by count id
  //
  // IF param edit is for a node defined outside voice, simply
  // look up the voice instances using that node and do the above

  return bindings;
}

int VoiceManager::allocateVoice(int templateId) {
  if (freeVoiceIds.empty())
    return -1;
  if (templateId < 0 || templateId >= static_cast<int>(voiceTemplates.size()))
    return -1;

  int voiceId = freeVoiceIds.front();
  freeVoiceIds.pop();

  auto instance = std::make_unique<VoiceInstance>();
  instance->setIds(voiceId, templateId);
  instance->setState(VoiceState::Active);

  // create nodes according to specs
  std::vector<int> nodeIds = instantiateNodes(templateId);

  // create and store param bindings
  std::vector<ParamBinding> bindings = instantiateParams(templateId);

  instance->setNodeIds(std::move(nodeIds));
  instance->setBindings(std::move(bindings));
  voiceInstances[voiceId] = std::move(instance);

  return voiceId;
}

void VoiceManager::freeVoice(int voiceId) {
  if (voiceId < 0 || voiceId >= maxVoices)
    return;

  // Remove per voice nodes from graph (should sort per removal)
  VoiceInstance &instance = *voiceInstances[voiceId];
  std::vector<std::unique_ptr<Node>> &nodes = graph.getNodes();

  for (int nodeId : instance.getNodeIds()) {
    if (nodes[nodeId]->syncMode.load(std::memory_order_relaxed) ==
        SyncMode::PerVoice) {
      graph.removeNode(nodeId);
    }
  }

  voiceInstances[voiceId].reset();
  freeVoiceIds.push(voiceId);
}

void VoiceManager::freeAllVoices() {
  while (!freeVoiceIds.empty())
    freeVoiceIds.pop();

  for (int i = 0; i < maxVoices; ++i) {
    voiceInstances[i].reset();
    freeVoiceIds.push(i);
  }
}
