declare name      "Blitz";
declare version   "1.0";
declare author    "Stefan Kersten";
declare license   "GPL";
declare copyright "Copyright (c) Stefan Kersten 2007-2008";

import("skOsc.lib");

process(f) = blit(f, n)
    with {
        n = floor(SR/(2*f));
    };
