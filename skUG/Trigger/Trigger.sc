SendTrigN : UGen {
	*ar { arg in = 0.0, id = 0 ... values;
		this.multiNew('audio', in, id, *values);
		^0.0		// SendTrig has no output
	}
	*kr { arg in = 0.0, id = 0 ... values;
		this.multiNew('control', in, id, *values);
		^0.0		// SendTrig has no output
	}
 	checkInputs { ^this.checkSameRateAsFirstInput }
	numOutputs { ^0 }
	writeOutputSpecs {}
}
