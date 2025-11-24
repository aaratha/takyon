#include "graph.h"

int Graph::addNode(unique_ptr<Node> node) {
  // check if any unallocated node slots exist
  if (freeIDs.size() > 0) {
    int id = freeIDs.front();
    freeIDs.pop();
    nodes[id] = std::move(node);
    return id;
  } else {

    // allocate new slot if all are used
    nodes.push_back(std::move(node));
    parents.push_back(vector<int>());
    children.push_back(vector<int>());
    return nodes.size() - 1;
  }
}

void Graph::removeNode(int id) {
  nodes[id].reset();
  freeIDs.push(id);

  // iterate parents and remove this node ID
  for (int pID : parents[id]) {
    auto &sibs = children[pID];
    sibs.erase(std::remove(sibs.begin(), sibs.end(), id), sibs.end());
  }

  // clear parents vector
  parents[id].clear();

  // iterate children and remove this node ID
  for (int cID : children[id]) {
    auto &sibs = parents[cID];
    sibs.erase(std::remove(sibs.begin(), sibs.end(), id), sibs.end());
  }

  // clear children vector
  children[id].clear();

  sort();
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

vector<unique_ptr<Node>> &Graph::getNodes() { return nodes; }
vector<int> &Graph::getTopoOrder() { return topoOrder; }
vector<int> &Graph::getSinkedNodes() { return sinkedNodes; }
