declare name      "ButterHP6";
declare version   "1.0";
declare author    "Stefan Kersten";
declare license   "GPL";
declare copyright "Copyright (c) Stefan Kersten 2008";

import("skFilter.lib");

process(x,f,r) = butter_hp_6(f,r,x);
