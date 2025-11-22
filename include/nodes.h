#pragma once

#include "graph.h"

using namespace std;

struct ControlNode : Node {
  vector<atomic<float> *> targets;
};
