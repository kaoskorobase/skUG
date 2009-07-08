{
	var in = SoundIn.ar(0);
	var freq, hasFreq, z, amp;
	#freq, hasFreq = Pitch.kr(in);
	amp = Amplitude.ar(in, releaseTime: 0.2);
	z = SinINLF.ar(BrownNoise.ar(0.1)+SinOsc.ar(freq)+SinOsc.ar(freq/2)*0.5, MouseX.kr, 0.9, 1.3) * amp.squared;
	z.dup
}.play;
