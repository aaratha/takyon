#include "audio.h"
#include "graph.h"
#include "lua_engine.h"
#include "nodes.h"

int main(int argc, char **argv) {
  Graph graph;
  AudioEngine aEngine(graph);
  LuaEngine lEngine(graph, aEngine);

  if (argc > 1) {
    std::string filename = argv[1];
    lEngine.runFile(filename);
  }

  lEngine.loop();
}
