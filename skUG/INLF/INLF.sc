SinINLF : UGen {
	*ar { | in, ratio(1) ... amps |
		^this.multiNew('audio', in, ratio, *amps)
	}
}