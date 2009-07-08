module Sound.SC3.UGen.SKUG.FM7 (
    fm7
) where

import Sound.SC3.UGen hiding (fm7)

listFrom3 :: (a, a, a) -> [a]
listFrom3 (a1, a2, a3) = [a1, a2, a3]

listFrom6 :: (a, a, a, a, a, a) -> [a]
listFrom6 (a1, a2, a3, a4, a5, a6) = [a1, a2, a3, a4, a5, a6]

type Control            = (UGen, UGen, UGen)
type ControlMatrix      = (Control, Control, Control, Control, Control, Control)
type Modulator          = (UGen, UGen, UGen, UGen, UGen, UGen)
type ModulatorMatrix    = (Modulator, Modulator, Modulator, Modulator, Modulator, Modulator)

fm7 :: ControlMatrix -> ModulatorMatrix -> UGen
fm7 ctrlMatrix modMatrix = mkOsc AR "FM7" args 6
    where args =    concat (map listFrom3 (listFrom6 ctrlMatrix))
                 ++ concat (map listFrom6 (listFrom6 modMatrix))
