# Takyon - README

Modular Synthesizer Live Coding Framework in Lua

## Example

```lua
BPM = 120
BPS = BPM / 60

-- Note = lfo(45, 5, BPS / 4, 0, Square)
-- O = osc(0.1, Note, Saw)
-- Env = lfo(0.05, 0.05, BPS, 0, InvSaw)
-- FEnv = lfo(1000, 1000, BPS, 0, InvSaw)
-- F = filter(FEnv, 2)
--
-- sound(O).amp(Env).effect(F).play()
--
-- SAME AS BELOW:

Base = osc(0.1, 0, Saw)
sound(Base)
 .amp(lfo(0.05, 0.05, BPS, 0, InvSaw))
 .freq(lfo(45, 5, BPS / 4, 0, Square))
 .effect(filter(0, 2))
 .cutoff(lfo(1000, 1000, BPS, 0, InvSaw))
 .play()

Main = osc(0.1, 200, Sine)
sound(Main).amp(lfo(0.05, 0.05, BPS * 2, 0, Square)).play()
```
