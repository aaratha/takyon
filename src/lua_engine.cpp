#include "lua_engine.h"

#include <filesystem>
#include <fstream>
#include <iostream>

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

// TODO
void LuaEngine::reloadFile(const std::filesystem::path &path) {
  // Two potential methods:
  // 1. loop over lines/expressions and compare to saved state
  // 2. create and overwrite objects with changes
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
  std::string line;
  while (true) {
    std::cout << "-> ";
    if (!std::getline(std::cin, line))
      break;
    if (line == "exit")
      break;
    runString(line);
  }
}
