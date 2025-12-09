#pragma once

#include <atomic>
#include <functional>
#include <queue>
#include <vector>

enum class SyncMode { PerVoice, Shared };

struct Node {
  std::atomic<bool> sinked = false; // audioOut
  std::atomic<float> out{0.0f};
  std::atomic<SyncMode> syncMode{SyncMode::PerVoice};

  virtual ~Node() = default;
  virtual void update() = 0;
};

struct Param {
  std::atomic<float> *ptr;
  Node *owner;
};

class Graph {
  std::vector<std::unique_ptr<Node>> nodes;
  std::queue<int> freeIDs;
  std::vector<std::vector<int>> parents;
  std::vector<std::vector<int>> children;
  std::vector<int> topoOrder; // cached
  std::vector<int> sinkedNodes;

public:
  Graph() = default;
  ~Graph() = default;

  int addNode(std::unique_ptr<Node> node);
  void removeNode(int id);

  void addEdge(int parent, int child);

  void sort(); // pass to topoOrder
  void traverse(const std::function<void(Node *)> &func);

  std::vector<std::unique_ptr<Node>> &getNodes();
  std::vector<int> &getTopoOrder();
  std::vector<int> &getSinkedNodes();
};
