-- create node objects
O = osc(0.1, 440, Saw)
X = osc(0.1, 220, Sine)
L1 = lfo(440, 220, 2)
L2 = lfo(2, 2, 1)
L3 = lfo(0.05, 0.05, 2, lfo(PI / 2, PI / 2, 0.25, 0, Sine), Square)
-- or L3 = lfo(0.05, 0.05, 2, L4, Square)
L4 = lfo(PI / 2, PI / 2, 0.25, 0, Sine)
L5 = lfo(1000, 700, 0.25)
F = filter(1000, 2)

-- edit params with new indexes or control nodes
O.type = Square

-- build sound, play if ends with play()
-- plug control nodes into params
-- pipe sound through effects, editing those params as well
sound(O).freq(lfo(440, 220, 2).freq(L2)).amp(L3.shift(L4)).amp(L3.shift(L4)).amp(L3.shift(L4)).effect(F).cutoff(L5)
-- .play()
--

LL = lfo(100, 100, 0.1)
LO = lfo(330, 110, LL)
X = osc(0.1, LO, Saw)
L = lfo(3000, 3000, 0.25)
F1 = filter(L, 2) -- This should work - passing LFO as cutoff parameter
sound(X).effect(F1).play()

