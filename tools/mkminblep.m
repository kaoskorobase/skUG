fc = 1;
Nzc = 16;
omega = 64;
thresh = -96;

mbtable = minblep( fc, Nzc, omega, thresh );
mblen = length( mbtable );
save -binary mbtable.mat mbtable ktable nzc mblen;
