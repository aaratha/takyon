#include "graph.h"

#include <queue>

int Graph::addNode(unique_ptr<Node> node) {
  nodes.push_back(node);
  parents.push_back(vector<int>());
  children.push_back(vector<int>());
  return nodes.size() - 1;
}

void Graph::addEdge(int parent, int child) {
  parents[child].push_back(parent);
  children[parent].push_back(child);
}

void Graph::sort() {
  unordered_map<int, int> inDegree;
  for (int i = 0; i < nodes.size(); i++) {
    inDegree[i] = parents[i].size();
  }

  queue<int> q;
  for (auto &[node, deg] : inDegree) {
    if (deg == 0)
      q.push(node);
  }

  vector<int> order;
  while (!q.empty()) {
    int node = q.front();
    q.pop();
    order.push_back(node);

    for (int child : children[node]) {
      inDegree[child]--;
      if (inDegree[child] == 0) {
        q.push(child);
      }
    }
  }

  if (order.size() != nodes.size())
    throw std::runtime_error("Graph has cycles!");

  topoOrder = order;
}
