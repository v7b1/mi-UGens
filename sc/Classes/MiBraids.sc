// volker böhm, 2020 - https://vboehm.net

MiBraids : UGen {

	*ar {
		arg pitch=60.0, timbre=0.5, color=0.5, model=0, trig=0, resamp=0,
		decim=0, bits=0, ws=0, mul=1.0;
		^this.multiNew('audio', pitch, timbre, color, model, trig, resamp, decim, bits, ws).madd(mul);
	}

}