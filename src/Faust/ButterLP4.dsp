declare name      "ButterLP4";
declare version   "1.0";
declare author    "Stefan Kersten";
declare license   "GPL";
declare copyright "Copyright (c) Stefan Kersten 2008";

import("skDSP.lib");
import("skFilter.lib");

process(x,f,r) = butter_lp_4(f,r,x);
