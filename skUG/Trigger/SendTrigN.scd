OSCresponder(s.addr, "/tr", { |...args| args[2].postln; }).add;

{ SendTrigN.ar(Impulse.ar(1), 0, *Line.ar(0, 10, 10).dup(4)) }.play;

b = Buffer.alloc(s, 1024, 1); //for sampling rates 44100 and 48000

(
x = {
	var in, buf, fft, array;
		
	in = SoundIn.ar(0); 
	buf = LocalBuf(1024, 1);
	fft = FFT(b, in);
	
	array = MFCC.kr(fft); 
	
	array.size.postln; 

	SendTrigN.ar(Impulse.ar(10), 0, *array);	
}.play
)
