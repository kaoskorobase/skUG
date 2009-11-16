module Sound.SC3.UGen.SKUG.VAD (
    vad
) where

import Sound.SC3.UGen

vad :: UGen -> UGen
vad x = mkOsc AR "VAD" [x] 2
