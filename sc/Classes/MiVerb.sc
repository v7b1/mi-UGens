// volker böhm, 2020 - https://vboehm.net

MiVerb : MultiOutUGen {

	*ar {
		arg inputArray, time=0.7, amount=0.5, diff=0.7, damp=0.5, freeze=0, mul=1.0, add=0.0;
		^this.multiNewList(['audio', time, amount, diff, 1.0-damp, freeze] ++ inputArray.asArray).madd(mul);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }
	checkInputs {
		var numArgs = 5;
		var numAudioInputs = this.numInputs - numArgs;

		numAudioInputs.do( { |i|
			var index = numArgs+i;
			if ( inputs.at(index).rate != 'audio', {
				^("input is not audio rate:" + inputs.at(index) + inputs.at(index).rate);
			});
		});
		/*
		if ( inputs.at(5).rate != 'audio', {
			^("input is not audio rate:" + inputs.at(5) + inputs.at(5).rate);
		});*/
		^this.checkValidInputs;
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}