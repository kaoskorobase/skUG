import("../skDSP.lib");

mod1(x,y) = if(y == 0., 0., r)
    with {
    	p = (x < 0.) | (y <= x);
    	r = x - float(p)*y*floor(x/y);
    };

mod2(x,y) = if(y == 0., 0., if(p, r, y))
    with {
    	p = (0. > x) & (x >= y);
    	r = x - y*floor(x/y);
    };

mod3(x,h) = if (r == 0.0, l, y)
    with {
        l = 0.0;
        r = h;
    	p = (x < l) | (h <= x);
        y = x - float(p)*trunc(x-l, r);
    };

process = mod1;
