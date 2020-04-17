// volker böhm, 2019 - https://vboehm.net

MiPlaits : MultiOutUGen {

	*ar {
		arg pitch=60.0, engine=0, harm=0.1, timbre=0.5, morph=0.5, trigger=0.0, level=0, fm_mod=0.0, timb_mod=0.0,
		morph_mod=0.0, decay=0.5, lpg_colour=0.5, mul=1.0;
		^this.multiNew('audio', pitch, engine, harm, timbre, morph, trigger, level, fm_mod, timb_mod, morph_mod,
			decay, lpg_colour).madd(mul);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}