local raw = {
  osc = osc,
  lfo = lfo,
  filter = filter,
  sound = sound,
}

local function is_plain_table(value)
  if type(value) ~= "table" then
    return false
  end
  local mt = getmetatable(value)
  return mt == nil
end

local function parse_params(spec, ...)
  local args = { ... }
  local first = args[1]

  if is_plain_table(first) then
    local cfg = {}
    for key, default in pairs(spec.defaults) do
      cfg[key] = first[key]
      if cfg[key] == nil then
        local alias = spec.alias and spec.alias[key]
        cfg[key] = (alias and first[alias]) or default
      end
    end
    return cfg
  end

  local cfg = {}
  for _, key in ipairs(spec.order) do
    cfg[key] = spec.defaults[key]
  end
  for idx, key in ipairs(spec.order) do
    if args[idx] ~= nil then
      cfg[key] = args[idx]
    end
  end
  return cfg
end

-- Check if a value is a control node (LFO)
local function is_control_node(value)
  if type(value) ~= "userdata" then
    return false
  end
  local mt = getmetatable(value)
  if not mt then return false end
  return tostring(mt) == "takyon.lfo"
end

local oscSpec = {
  defaults = { amp = 1.0, freq = 440.0, type = Sine },
  order = { "amp", "freq", "type" },
}

local lfoSpec = {
  defaults = { base = 0.0, amp = 1.0, freq = 5.0, shift = 0.0, type = Sine },
  order = { "base", "amp", "freq", "shift", "type" },
}

local filterSpec = {
  defaults = { cutoff = 1000.0, q = 1.0 },
  order = { "cutoff", "q" },
}

function osc(...)
  local cfg = parse_params(oscSpec, ...)
  return raw.osc(cfg.amp, cfg.freq, cfg.type)
end

function lfo(...)
  local cfg = parse_params(lfoSpec, ...)
  return raw.lfo(cfg.base, cfg.amp, cfg.freq, cfg.shift, cfg.type)
end

function filter(...)
  local cfg = parse_params(filterSpec, ...)
  return raw.filter(cfg.cutoff, cfg.q)
end

local function apply_steps(builder, steps)
  if not steps then
    return builder
  end
  for _, step in ipairs(steps) do
    if type(step) == "function" then
      builder = step(builder) or builder
    end
  end
  return builder
end

function sound(arg)
  if is_plain_table(arg) then
    assert(arg.source, "sound() table form expects 'source'")
    local builder = raw.sound(arg.source)
    builder = apply_steps(builder, arg.chain)
    if arg.auto_play then
      builder:play()
    end
    return builder
  end
  return raw.sound(arg)
end

local function chain(node, ...)
  local builder = sound(node)
  for _, step in ipairs({ ... }) do
    if type(step) == "function" then
      builder = step(builder) or builder
    end
  end
  return builder
end

local Helpers = {
  raw = raw,
  chain = chain,
  apply = function(builder, fn)
    return (fn and fn(builder)) or builder
  end,
}

return Helpers
