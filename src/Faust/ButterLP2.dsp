declare name      "ButterLP2";
declare version   "1.0";
declare author    "Stefan Kersten";
declare license   "GPL";
declare copyright "Copyright © Stefan Kersten 2008";

import("skDSP.lib");
import("skFilter.lib");

process(x,f,r) = butter_lp_2(f,r,x);
