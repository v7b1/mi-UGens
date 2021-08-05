// https://vboehm.net

MiGrids : MultiOutUGen {
	*ar {
		arg bpm=120, map_x = 0.5, map_y = 0.5, chaos = 0,
		bd_density = 0.25, sd_density = 0.25, hh_density = 0.25,
		mode = 0, swing = 0, config = 0, reso = 2;
		^this.multiNew('audio', bpm, map_x, map_y, chaos, bd_density, sd_density, hh_density,
			mode, swing, config, reso);
	}
	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(6, rate);
	}
}
