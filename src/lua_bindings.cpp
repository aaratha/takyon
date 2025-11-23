#include "lua_bindings.h"

#include "globals.h"
#include "nodes.h"

#include <algorithm>
#include <cmath>
#include <cstring>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

namespace {

constexpr const char *OSC_MT = "takyon.osc";
constexpr const char *LFO_MT = "takyon.lfo";
constexpr const char *FILTER_MT = "takyon.filter";
constexpr const char *BUILDER_MT = "takyon.sound_builder";

struct LuaNodeHandle {
  LuaContext *ctx{};
  int nodeId = -1;
};

struct LuaSoundBuilder {
  LuaContext *ctx{};
  int sourceId = -1;  // oscillator or initial audio node
  int currentId = -1; // tip of the effect chain
};

LuaContext *getCtx(lua_State *L) {
  return static_cast<LuaContext *>(lua_touserdata(L, lua_upvalueindex(1)));
}

int call_with_self(lua_State *L) {
  lua_CFunction method = lua_tocfunction(L, lua_upvalueindex(1));
  lua_pushvalue(L, lua_upvalueindex(2)); // self
  lua_insert(L, 1);
  return method(L);
}

int push_method_closure(lua_State *L, const char *mtName) {
  const char *field = luaL_checkstring(L, 2);

  luaL_getmetatable(L, mtName);
  lua_getfield(L, -1, "__methods");
  lua_getfield(L, -1, field);

  if (lua_isnil(L, -1)) {
    lua_pop(L, 3);
    lua_pushnil(L);
    return 1;
  }

  if (!lua_iscfunction(L, -1)) {
    lua_pop(L, 3);
    lua_pushnil(L);
    return 1;
  }

  lua_pushvalue(L, -1); // method upvalue
  lua_pushvalue(L, 1);  // self upvalue
  lua_pushcclosure(L, call_with_self, 2);

  lua_remove(L, -2); // remove raw method
  lua_pop(L, 2);     // pop methods table + metatable

  return 1;
}

Graph &getGraphOrThrow(lua_State *L, LuaContext *ctx) {
  if (!ctx || !ctx->graph)
    luaL_error(L, "Graph is not available");
  return *ctx->graph;
}

template <typename T>
T *getNodeAs(lua_State *L, Graph &graph, int nodeId, const char *typeName) {
  auto &nodes = graph.getNodes();
  if (nodeId < 0 || nodeId >= static_cast<int>(nodes.size()) ||
      nodes[nodeId] == nullptr) {
    luaL_error(L, "Invalid %s handle", typeName);
  }
  auto *typed = dynamic_cast<T *>(nodes[nodeId].get());
  if (!typed)
    luaL_error(L, "Handle is not a %s", typeName);
  return typed;
}

LuaNodeHandle *pushNodeHandle(lua_State *L, LuaContext *ctx, int nodeId,
                              const char *mtName) {
  auto *handle =
      static_cast<LuaNodeHandle *>(lua_newuserdata(L, sizeof(LuaNodeHandle)));
  handle->ctx = ctx;
  handle->nodeId = nodeId;
  luaL_getmetatable(L, mtName);
  lua_setmetatable(L, -2);
  return handle;
}

LuaNodeHandle *checkNodeHandle(lua_State *L, int index,
                               const char *mtName) {
  return static_cast<LuaNodeHandle *>(luaL_checkudata(L, index, mtName));
}

LuaSoundBuilder *pushBuilder(lua_State *L, LuaContext *ctx, int sourceId) {
  auto *builder = static_cast<LuaSoundBuilder *>(
      lua_newuserdata(L, sizeof(LuaSoundBuilder)));
  builder->ctx = ctx;
  builder->sourceId = sourceId;
  builder->currentId = sourceId;
  luaL_getmetatable(L, BUILDER_MT);
  lua_setmetatable(L, -2);
  return builder;
}

LuaSoundBuilder *checkBuilder(lua_State *L, int index) {
  return static_cast<LuaSoundBuilder *>(luaL_checkudata(L, index, BUILDER_MT));
}

Waveform toWaveform(lua_State *L, int index) {
  if (lua_isnoneornil(L, index))
    return Waveform::Sine;
  int wf = static_cast<int>(luaL_checkinteger(L, index));
  if (wf < static_cast<int>(Waveform::Sine) ||
      wf > static_cast<int>(Waveform::Triangle)) {
    luaL_error(L, "Invalid waveform id %d", wf);
  }
  return static_cast<Waveform>(wf);
}

bool isControlHandle(lua_State *L, int index) {
  return luaL_testudata(L, index, LFO_MT) != nullptr;
}

void attachControl(lua_State *L, LuaNodeHandle *owner, int ownerId,
                   atomic<float> &param, int controlIndex) {
  auto *controlHandle =
      static_cast<LuaNodeHandle *>(luaL_checkudata(L, controlIndex, LFO_MT));
  auto *ctx = owner->ctx;
  Graph &graph = getGraphOrThrow(L, ctx);
  auto *control =
      getNodeAs<ControlNode>(L, graph, controlHandle->nodeId, "control node");
  control->addTarget(&param);
  graph.addEdge(controlHandle->nodeId, ownerId);
}

void setScalarOrControl(lua_State *L, LuaNodeHandle *owner,
                        atomic<float> &param, int valueIndex,
                        bool allowControl) {
  if (allowControl && isControlHandle(L, valueIndex)) {
    attachControl(L, owner, owner->nodeId, param, valueIndex);
    return;
  }
  float value = static_cast<float>(luaL_checknumber(L, valueIndex));
  param.store(value, std::memory_order_relaxed);
}

void registerWaveformGlobals(lua_State *L) {
  lua_pushinteger(L, static_cast<int>(Waveform::Sine));
  lua_setglobal(L, "Sine");
  lua_pushinteger(L, static_cast<int>(Waveform::Saw));
  lua_setglobal(L, "Saw");
  lua_pushinteger(L, static_cast<int>(Waveform::InvSaw));
  lua_setglobal(L, "InvSaw");
  lua_pushinteger(L, static_cast<int>(Waveform::Square));
  lua_setglobal(L, "Square");
  lua_pushinteger(L, static_cast<int>(Waveform::Triangle));
  lua_setglobal(L, "Triangle");

  constexpr double PI = 3.14159265358979323846;
  lua_pushnumber(L, static_cast<lua_Number>(PI));
  lua_setglobal(L, "PI");
}

// --- Oscillator methods -----------------------------------------------------

int osc_freq(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, OSC_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *osc = getNodeAs<Oscillator>(L, graph, handle->nodeId, "oscillator");
  setScalarOrControl(L, handle, osc->freq, 2, true);
  lua_settop(L, 1);
  return 1;
}

int osc_amp(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, OSC_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *osc = getNodeAs<Oscillator>(L, graph, handle->nodeId, "oscillator");
  setScalarOrControl(L, handle, osc->amp, 2, true);
  lua_settop(L, 1);
  return 1;
}

int osc_type(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, OSC_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *osc = getNodeAs<Oscillator>(L, graph, handle->nodeId, "oscillator");
  Waveform wf = toWaveform(L, 2);
  osc->type.store(wf, std::memory_order_relaxed);
  lua_settop(L, 1);
  return 1;
}

int osc_newindex(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, OSC_MT);
  const char *field = luaL_checkstring(L, 2);
  if (std::strcmp(field, "freq") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return osc_freq(L);
  }
  if (std::strcmp(field, "amp") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return osc_amp(L);
  }
  if (std::strcmp(field, "type") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return osc_type(L);
  }
  return luaL_error(L, "unknown oscillator field '%s'", field);
}

const luaL_Reg oscMethods[] = {
    {"freq", osc_freq}, {"amp", osc_amp}, {"type", osc_type}, {nullptr, nullptr}};

int osc_index(lua_State *L) { return push_method_closure(L, OSC_MT); }

void createOscMetatable(lua_State *L) {
  if (luaL_newmetatable(L, OSC_MT)) {
    lua_newtable(L);
    luaL_setfuncs(L, oscMethods, 0);
    lua_setfield(L, -2, "__methods");
    lua_pushcfunction(L, osc_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, osc_newindex);
    lua_setfield(L, -2, "__newindex");
  }
  lua_pop(L, 1);
}

// --- LFO methods ------------------------------------------------------------

int lfo_base(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, LFO_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *lfo = getNodeAs<LFO>(L, graph, handle->nodeId, "lfo");
  setScalarOrControl(L, handle, lfo->base, 2, true);
  lua_settop(L, 1);
  return 1;
}

int lfo_amp(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, LFO_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *lfo = getNodeAs<LFO>(L, graph, handle->nodeId, "lfo");
  setScalarOrControl(L, handle, lfo->amp, 2, true);
  lua_settop(L, 1);
  return 1;
}

int lfo_freq(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, LFO_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *lfo = getNodeAs<LFO>(L, graph, handle->nodeId, "lfo");
  setScalarOrControl(L, handle, lfo->freq, 2, true);
  lua_settop(L, 1);
  return 1;
}

int lfo_shift(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, LFO_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *lfo = getNodeAs<LFO>(L, graph, handle->nodeId, "lfo");
  setScalarOrControl(L, handle, lfo->shift, 2, true);
  lua_settop(L, 1);
  return 1;
}

int lfo_type(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, LFO_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *lfo = getNodeAs<LFO>(L, graph, handle->nodeId, "lfo");
  Waveform wf = toWaveform(L, 2);
  lfo->type.store(wf, std::memory_order_relaxed);
  lua_settop(L, 1);
  return 1;
}

int lfo_newindex(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, LFO_MT);
  const char *field = luaL_checkstring(L, 2);
  if (std::strcmp(field, "base") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return lfo_base(L);
  }
  if (std::strcmp(field, "amp") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return lfo_amp(L);
  }
  if (std::strcmp(field, "freq") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return lfo_freq(L);
  }
  if (std::strcmp(field, "shift") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return lfo_shift(L);
  }
  if (std::strcmp(field, "type") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return lfo_type(L);
  }
  return luaL_error(L, "unknown LFO field '%s'", field);
}

const luaL_Reg lfoMethods[] = {{"base", lfo_base},   {"amp", lfo_amp},
                               {"freq", lfo_freq},   {"shift", lfo_shift},
                               {"type", lfo_type},   {nullptr, nullptr}};

int lfo_index(lua_State *L) { return push_method_closure(L, LFO_MT); }

void createLfoMetatable(lua_State *L) {
  if (luaL_newmetatable(L, LFO_MT)) {
    lua_newtable(L);
    luaL_setfuncs(L, lfoMethods, 0);
    lua_setfield(L, -2, "__methods");
    lua_pushcfunction(L, lfo_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, lfo_newindex);
    lua_setfield(L, -2, "__newindex");
  }
  lua_pop(L, 1);
}

// --- Filter methods ---------------------------------------------------------

int filter_cutoff(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, FILTER_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *filter = getNodeAs<Filter>(L, graph, handle->nodeId, "filter");
  setScalarOrControl(L, handle, filter->cutoff, 2, true);
  lua_settop(L, 1);
  return 1;
}

int filter_q(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, FILTER_MT);
  Graph &graph = getGraphOrThrow(L, handle->ctx);
  auto *filter = getNodeAs<Filter>(L, graph, handle->nodeId, "filter");
  setScalarOrControl(L, handle, filter->q, 2, true);
  lua_settop(L, 1);
  return 1;
}

int filter_newindex(lua_State *L) {
  auto *handle = checkNodeHandle(L, 1, FILTER_MT);
  const char *field = luaL_checkstring(L, 2);
  if (std::strcmp(field, "cutoff") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return filter_cutoff(L);
  }
  if (std::strcmp(field, "q") == 0) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_replace(L, 2);
    return filter_q(L);
  }
  return luaL_error(L, "unknown filter field '%s'", field);
}

const luaL_Reg filterMethods[] = {
    {"cutoff", filter_cutoff}, {"q", filter_q}, {nullptr, nullptr}};

int filter_index(lua_State *L) { return push_method_closure(L, FILTER_MT); }

void createFilterMetatable(lua_State *L) {
  if (luaL_newmetatable(L, FILTER_MT)) {
    lua_newtable(L);
    luaL_setfuncs(L, filterMethods, 0);
    lua_setfield(L, -2, "__methods");
    lua_pushcfunction(L, filter_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, filter_newindex);
    lua_setfield(L, -2, "__newindex");
  }
  lua_pop(L, 1);
}

// --- Sound builder methods --------------------------------------------------

Oscillator *resolveBuilderOsc(lua_State *L, LuaSoundBuilder *builder) {
  Graph &graph = getGraphOrThrow(L, builder->ctx);
  return getNodeAs<Oscillator>(L, graph, builder->sourceId, "oscillator");
}

Node *resolveBuilderTip(lua_State *L, LuaSoundBuilder *builder) {
  Graph &graph = getGraphOrThrow(L, builder->ctx);
  auto &nodes = graph.getNodes();
  if (builder->currentId < 0 ||
      builder->currentId >= static_cast<int>(nodes.size()) ||
      nodes[builder->currentId] == nullptr) {
    luaL_error(L, "Builder has invalid node");
  }
  return nodes[builder->currentId].get();
}

int builder_freq(lua_State *L) {
  auto *builder = checkBuilder(L, 1);
  auto *osc = resolveBuilderOsc(L, builder);
  LuaNodeHandle fakeHandle{builder->ctx, builder->sourceId};
  setScalarOrControl(L, &fakeHandle, osc->freq, 2, true);
  lua_settop(L, 1);
  return 1;
}

int builder_amp(lua_State *L) {
  auto *builder = checkBuilder(L, 1);
  auto *osc = resolveBuilderOsc(L, builder);
  LuaNodeHandle fakeHandle{builder->ctx, builder->sourceId};
  setScalarOrControl(L, &fakeHandle, osc->amp, 2, true);
  lua_settop(L, 1);
  return 1;
}

int builder_effect(lua_State *L) {
  auto *builder = checkBuilder(L, 1);
  auto *effectHandle = checkNodeHandle(L, 2, FILTER_MT);
  Graph &graph = getGraphOrThrow(L, builder->ctx);
  auto *effect = getNodeAs<EffectNode>(L, graph, effectHandle->nodeId, "effect");
  Node *upstream = resolveBuilderTip(L, builder);
  effect->addInput(&upstream->out);
  graph.addEdge(builder->currentId, effectHandle->nodeId);
  builder->currentId = effectHandle->nodeId;
  lua_settop(L, 1);
  return 1;
}

int builder_cutoff(lua_State *L) {
  auto *builder = checkBuilder(L, 1);
  Graph &graph = getGraphOrThrow(L, builder->ctx);
  auto *filter = getNodeAs<Filter>(L, graph, builder->currentId, "filter");
  LuaNodeHandle fakeHandle{builder->ctx, builder->currentId};
  setScalarOrControl(L, &fakeHandle, filter->cutoff, 2, true);
  lua_settop(L, 1);
  return 1;
}

int builder_play(lua_State *L) {
  auto *builder = checkBuilder(L, 1);
  Graph &graph = getGraphOrThrow(L, builder->ctx);
  auto &nodes = graph.getNodes();
  if (builder->currentId < 0 ||
      builder->currentId >= static_cast<int>(nodes.size()) ||
      nodes[builder->currentId] == nullptr) {
    return luaL_error(L, "Cannot play: invalid node");
  }

  nodes[builder->currentId]->sinked.store(true, std::memory_order_relaxed);
  auto &sinks = graph.getSinkedNodes();
  if (std::find(sinks.begin(), sinks.end(), builder->currentId) == sinks.end())
    sinks.push_back(builder->currentId);
  graph.sort();
  return 0;
}

const luaL_Reg builderMethods[] = {{"freq", builder_freq},
                                   {"amp", builder_amp},
                                   {"effect", builder_effect},
                                   {"cutoff", builder_cutoff},
                                   {"play", builder_play},
                                   {nullptr, nullptr}};

int builder_index(lua_State *L) { return push_method_closure(L, BUILDER_MT); }

void createBuilderMetatable(lua_State *L) {
  if (luaL_newmetatable(L, BUILDER_MT)) {
    lua_newtable(L);
    luaL_setfuncs(L, builderMethods, 0);
    lua_setfield(L, -2, "__methods");
    lua_pushcfunction(L, builder_index);
    lua_setfield(L, -2, "__index");
  }
  lua_pop(L, 1);
}

// --- Constructors -----------------------------------------------------------

int lua_create_osc(lua_State *L) {
  auto *ctx = getCtx(L);
  Graph &graph = getGraphOrThrow(L, ctx);

  float amp = static_cast<float>(luaL_optnumber(L, 1, 1.0));
  float freq = static_cast<float>(luaL_optnumber(L, 2, 440.0));
  Waveform type = toWaveform(L, 3);

  auto node = Oscillator::init(amp, freq, type);
  int id = graph.addNode(std::move(node));
  pushNodeHandle(L, ctx, id, OSC_MT);
  return 1;
}

int lua_create_lfo(lua_State *L) {
  auto *ctx = getCtx(L);
  Graph &graph = getGraphOrThrow(L, ctx);

  float base = static_cast<float>(luaL_optnumber(L, 1, 0.0));
  float amp = static_cast<float>(luaL_optnumber(L, 2, 1.0));
  float freq = static_cast<float>(luaL_optnumber(L, 3, 5.0));

  bool shiftIsControl = isControlHandle(L, 4);
  float shift = shiftIsControl
                    ? 0.0f
                    : static_cast<float>(luaL_optnumber(L, 4, 0.0));

  Waveform type = toWaveform(L, 5);

  auto node = LFO::init(base, amp, freq, shift, type);
  int id = graph.addNode(std::move(node));
  auto *handle = pushNodeHandle(L, ctx, id, LFO_MT);

  if (shiftIsControl) {
    setScalarOrControl(L, handle,
                       getNodeAs<LFO>(L, graph, id, "lfo")->shift, 4, true);
  }

  return 1;
}

int lua_create_filter(lua_State *L) {
  auto *ctx = getCtx(L);
  Graph &graph = getGraphOrThrow(L, ctx);

  float cutoff = static_cast<float>(luaL_optnumber(L, 1, 1000.0));
  float q = static_cast<float>(luaL_optnumber(L, 2, 1.0));

  auto node = Filter::init(cutoff, q);
  int id = graph.addNode(std::move(node));
  pushNodeHandle(L, ctx, id, FILTER_MT);
  return 1;
}

int lua_sound_builder(lua_State *L) {
  auto *ctx = getCtx(L);
  auto *sourceHandle = checkNodeHandle(L, 1, OSC_MT);
  pushBuilder(L, ctx, sourceHandle->nodeId);
  return 1;
}

} // namespace

void registerLuaBindings(lua_State *L, LuaContext *ctx) {
  registerWaveformGlobals(L);
  createOscMetatable(L);
  createLfoMetatable(L);
  createFilterMetatable(L);
  createBuilderMetatable(L);

  lua_pushlightuserdata(L, ctx);
  lua_pushcclosure(L, lua_create_osc, 1);
  lua_setglobal(L, "osc");

  lua_pushlightuserdata(L, ctx);
  lua_pushcclosure(L, lua_create_lfo, 1);
  lua_setglobal(L, "lfo");

  lua_pushlightuserdata(L, ctx);
  lua_pushcclosure(L, lua_create_filter, 1);
  lua_setglobal(L, "filter");

  lua_pushlightuserdata(L, ctx);
  lua_pushcclosure(L, lua_sound_builder, 1);
  lua_setglobal(L, "sound");
}
