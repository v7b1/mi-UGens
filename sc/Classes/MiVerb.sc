// volker böhm, 2020 - https://vboehm.net

MiVerb : MultiOutUGen {

	*ar {
		arg inputArray, time=0.7, drywet=0.5, damp=0.5, hp=0.05, freeze=0, diff=0.625, mul=1.0, add=0.0;
		^this.multiNewList(['audio', time, drywet, damp, hp, freeze, diff] ++ inputArray.asArray).madd(mul);
	}

	checkInputs {
		var numArgs = 6;
		var numAudioInputs = this.numInputs - numArgs;

		numAudioInputs.do( { |i|
			var index = numArgs+i;
			if ( inputs.at(index).rate != 'audio', {
				^("input is not audio rate:" + inputs.at(index) + inputs.at(index).rate);
			});
		});
		^this.checkValidInputs;
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}