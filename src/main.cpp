#include "audio.h"
#include "graph.h"
#include "lua_engine.h"
#include "nodes.h"
#include "pattern.h"

int main(int argc, char **argv) {
  Graph graph;
  AudioEngine aEngine(graph);
  PatternEngine pEngine;
  LuaEngine lEngine(graph, aEngine, pEngine);

  if (argc > 1) {
    std::string filename = argv[1];
    lEngine.runFile(filename);
  }

  lEngine.loop();
}
