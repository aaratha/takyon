#include "lua_engine.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "linenoise.h"

LuaEngine::LuaEngine(Graph &graph, AudioEngine &ae) : graph(graph), ae(ae) {
  L = luaL_newstate();
  luaL_openlibs(L);

  ctx.graph = &graph;
  ctx.audio = &ae;
  registerLuaBindings(L, &ctx);

  const std::filesystem::path runtimePath = "lua/runtime.lua";
  if (std::filesystem::exists(runtimePath)) {
    if (luaL_loadfile(L, runtimePath.c_str()) || lua_pcall(L, 0, 0, 0)) {
      std::cerr << "Lua runtime error: " << lua_tostring(L, -1) << std::endl;
      lua_pop(L, 1);
    }
  }
}

LuaEngine::~LuaEngine() {
  stopWatcher();
  if (L)
    lua_close(L);
}

void LuaEngine::bindFunction(const std::string &name, lua_CFunction fn) {
  lua_register(L, name.c_str(), fn);
}

void LuaEngine::runString(const std::string &code) {
  if (luaL_loadstring(L, code.c_str()) || lua_pcall(L, 0, 0, 0)) {
    std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 1);
  }
}

void LuaEngine::runFile(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (in) {
    std::cout << "--- " << path << " ---\n";
    std::cout << in.rdbuf(); // echo the file
    std::cout << "\n--- end ---\n";
  }
  if (luaL_loadfile(L, path.c_str()) || lua_pcall(L, 0, 0, 0)) {
    std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 1);
  }

  startWatcher(path);
}

void LuaEngine::reloadFile(const std::filesystem::path &path) {
  // Store the audio engine reference
  AudioEngine* ae_ptr = ctx.audio;

  // Clear all nodes from the graph to destroy all audio objects
  auto& nodes = graph.getNodes();
  for (size_t i = 0; i < nodes.size(); i++) {
    if (nodes[i]) {
      graph.removeNode(i);
    }
  }

  // Also clear the sinked nodes
  graph.getSinkedNodes().clear();

  // Clean up the current Lua state
  lua_close(L);

  // Create a fresh Lua state
  L = luaL_newstate();
  luaL_openlibs(L);

  // Re-initialize the context (graph is already cleared)
  ctx.graph = &graph;  // Use the same graph reference but now it's empty
  ctx.audio = ae_ptr;
  registerLuaBindings(L, &ctx);

  // Load the runtime file
  const std::filesystem::path runtimePath = "lua/runtime.lua";
  if (std::filesystem::exists(runtimePath)) {
    if (luaL_loadfile(L, runtimePath.c_str()) || lua_pcall(L, 0, 0, 0)) {
      std::cerr << "Lua runtime error: " << lua_tostring(L, -1) << std::endl;
      lua_pop(L, 1);
    }
  }

  // Now load and run the target file
  std::ifstream in(path);
  if (in) {
    std::cout << "--- reloading " << path << " ---\n";
  }
  if (luaL_loadfile(L, path.c_str()) || lua_pcall(L, 0, 0, 0)) {
    std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 1);
  }
}

void LuaEngine::startWatcher(const std::filesystem::path &path) {
  watchingFile.store(true);
  watchThread = std::thread([this, path] {
    auto last = std::filesystem::last_write_time(path);
    while (watchingFile.load(std::memory_order_relaxed)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      auto now = std::filesystem::last_write_time(path);
      if (now != last) {
        last = now;
        reloadFile(path); // signal main thread
      }
    }
  });
}

void LuaEngine::stopWatcher() {
  if (!watchingFile.exchange(false))
    return;
  if (watchThread.joinable())
    watchThread.join();
}

void LuaEngine::loop() {
  char *line;
  while (true) {
    line = linenoise("-> ");
    if (!line)
      break;

    if (luaL_loadstring(L, line) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
      printf("Error: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);
    }

    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}
