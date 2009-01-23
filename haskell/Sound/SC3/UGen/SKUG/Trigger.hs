module Sound.SC3.UGen.SKUG.Trigger (
    sendTrigN
) where

import Sound.SC3.UGen

-- | Send a trigger message from the server back to the all registered clients.
sendTrigN :: UGen -> UGen -> [UGen] -> UGen
sendTrigN i k vs = mkFilter "SendTrigN" ([i, k] ++ vs) 0
