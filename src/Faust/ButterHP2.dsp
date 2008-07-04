declare name      "ButterHP2";
declare version   "1.0";
declare author    "Stefan Kersten";
declare license   "GPL";
declare copyright "Copyright (c) Stefan Kersten 2008";

import("skDSP.lib");
import("skFilter.lib");

process(x,f,r) = butter_hp_2(f,r,x);
