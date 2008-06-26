// VEP binaural rendering engine
//
// Copyright (C) 2005-2006 Stefan Kersten <sk@k-hornz.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA

VEPConvolution : MultiOutUGen
{
	*ar { | inRef, kernel, kernelMaxSize(0), kernelOffset(0), kernelSize(0), kernelTrigger(0), minPartSize(0), maxPartSize(8192), numRTProcs(0) |
		var in = inRef.dereference;
		^this.multiNewList(['audio', in.size] ++ in ++ [kernel, kernelMaxSize, kernelOffset, kernelSize, kernelTrigger, minPartSize, maxPartSize, numRTProcs])
	}
	init { | argNumChannels ... theInputs |
		inputs = theInputs;
		^this.initOutputs(argNumChannels, rate)
	}
}

// EOF