#pragma once

#include "audio.h"
#include "graph.h"

extern "C" {
#include <lua.h>
}

struct LuaContext {
  Graph *graph;
  AudioEngine *audio;
};

void registerLuaBindings(lua_State *L, LuaContext *ctx);
