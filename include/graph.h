#pragma once

#include <atomic>
#include <vector>
#include <functional>

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
  vector < unique_ptr < Node >> nodes;
  vector<vector<int>> parents;
  vector<vector<int>> children;
  vector<int> topoOrder; // cached
  vector<int> sinkedNodes;

public:
  Graph() = default;
  ~Graph();

  int addNode(unique_ptr<Node> node);
  void addEdge(int parent, int child);

  void sort(); // pass to topoOrder
  void traverse(const function<void(Node *)> &func);
};
