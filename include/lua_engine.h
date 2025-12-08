#pragma once

#include "audio.h"
#include "graph.h"
#include "lua_bindings.h"
#include "pattern.h"

#include <filesystem>
#include <string>
#include <thread>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

class LuaEngine {
  Graph &graph;
  AudioEngine &ae;
  PatternEngine &pe;
  lua_State *L;
  LuaContext ctx{};

  std::thread watchThread;
  std::atomic<bool> watchingFile{false};

public:
  LuaEngine(Graph &graph, AudioEngine &ae, PatternEngine &pe);
  ~LuaEngine();

  void bindFunction(const std::string &name, lua_CFunction fn);

  void runString(const std::string &code);
  void runFile(const std::filesystem::path &path);
  void reloadFile(const std::filesystem::path &path);

  void startWatcher(const std::filesystem::path &path);
  void stopWatcher();

  void loop(); // use readString
};
