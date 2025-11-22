#pragma once

#include <atomic>
#include <functional>
#include <queue>
#include <vector>

using namespace std;

struct Node {
  virtual ~Node() = default;
  virtual void update() = 0;

  atomic<bool> sinked = false; // audioOut
  atomic<float> out{0.0f};
};

struct Param {
  atomic<float> *ptr;
  Node *owner;
};

class Graph {
  vector<unique_ptr<Node>> nodes;
  queue<int> freeIDs;
  vector<vector<int>> parents;
  vector<vector<int>> children;
  vector<int> topoOrder; // cached
  vector<int> sinkedNodes;

public:
  Graph() = default;
  ~Graph() = default;

  int addNode(unique_ptr<Node> node);
  void removeNode(int id);

  void addEdge(int parent, int child);

  void sort(); // pass to topoOrder
  void traverse(const function<void(Node *)> &func);

  vector<unique_ptr<Node>> &getNodes();
  vector<int> &getTopoOrder();
  vector<int> &getSinkedNodes();
};
