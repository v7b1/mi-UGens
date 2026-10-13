# Credits and history

## Émilie Gillet

MiBeads exists because of Émilie Gillet. She designed Beads, wrote every line of its signal processing, and then chose to give that code away. Everything you hear from this UGen, such as the grains, the delay, the tape and digital media, the reverb, and the wavetable synth, is her work. The SuperCollider wrapper only feeds it audio and controls and hands the result back.

The DSP sources in `eurorack/beads` and `eurorack/stmlib` are © Émilie Gillet and published under the MIT license. Her copyright notices are kept intact in every file.

- Source code: https://github.com/pichenettes/eurorack (Beads: `beads/`) and https://github.com/pichenettes/stmlib
- Documentation and manual: https://pichenettes.github.io/mutable-instruments-documentation/modules/beads/

## Mutable Instruments and open source

Mutable Instruments was Émilie Gillet's company. It began with the Shruthi-1, a monophonic synth that grew out of her experiments with Arduino boards around 2009, followed by the Ambika and the Anushri. She moved into Eurorack in 2012, and Braids followed in 2013. Clouds and Elements came in 2014, Rings in 2015, and Plaits in 2018. Many consider these some of the most influential modules the format has had.

From the Shruthi-1 onward she published the firmware and hardware designs of her instruments. Code for the STM32 based modules is MIT licensed, code for the older AVR projects is GPL 3.0, and the hardware is CC BY-SA 3.0. She has described this openness as a deliberate alternative to selling instruments on the promise of a secret "magic ingredient". Mutable's policy, announced in 2018, kept a new module's code closed at first, so that early clones would not hurt the dealers who carried it, and released it after the module had proven itself.

That generosity created an ecosystem far larger than the company:

- **VCV Rack.** VCV's Audible Instruments plugin brought Mutable's modules to a free, cross platform virtual modular. Plaits even appeared there as a software preview while the hardware was still new. For many people it was their first contact with these designs and with patching itself.
- **SuperCollider, Max and Tidal.** Volker Böhm ported the modules to SuperCollider as mi-UGens (https://github.com/v7b1/mi-UGens) and to Max as the vb.mi externals. The Tidal community built on mi-UGens, which brought Plaits, Rings, Clouds and the others into live coding. MiBeads follows the structure of his MiClouds directly. Without his groundwork this port would have had no template.
- **Alternative firmware.** Community firmware, such as the Parasites family for Clouds, Tides and other modules, extended the hardware in directions the original never shipped.
- **Hardware and instruments.** DIY builders and small makers such as Tall Dog Electronics and After Later Audio reproduced and adapted the designs. The DSP code also found its way into standalone instruments, including Arturia's MicroFreak and MiniFreak.

## Beads

Beads is the successor to Clouds. Rather than an update, it was a ground up rethinking of the granular texture synthesizer, with a crisper sound, more direct control and a delay mode. It was known along the way by working names such as Clouds 2, Crowds and Seeds. Hannes Pasqualini of papernoise, who shaped Mutable Instruments' visual identity and panels, was part of its design discussions.

Beads was announced in February 2021 and produced until December 2022. In April 2022 Émilie Gillet announced that no new modules were coming, writing that "Beads is the last module I designed". Mutable Instruments closed at the end of 2022.

For years Beads was the one Mutable module whose code was not public. On October 12, 2026 its source was published on the pichenettes GitHub account under the MIT license, which completed the open sourcing of the whole Mutable Instruments catalogue. MiBeads was written the same day as a direct result of that release.

## Respecting the license and the name

- The Beads code is used under its MIT license, with the copyright and permission notices kept in every source file.
- The changes made for this port are small, marked `MiBeads:` in the source, and listed in the README. They are portability fixes and do not change the sound. Output was verified bit identical to the original code.
- Mutable Instruments is a registered trademark. The eurorack repository asks that the name not be used on derivative works and recommends not reusing a module's name. MiBeads follows the long standing mi-UGens naming convention (MiClouds, MiRings and so on) and is not affiliated with or endorsed by Mutable Instruments.

## Other thanks

- **Volker Böhm** for mi-UGens and the MiClouds wrapper this port is modeled on.
- **papernoise** for the visual identity of Mutable Instruments.
- **VCV** for Audible Instruments, which showed how far these modules could travel beyond Eurorack.
- **Peter Kirn / CDM** and **Synth Anatomy** for reporting the Beads source release.
- **The SuperCollider developers and community.**

## Sources

- CDM, "Beads is now open source" (October 2026): https://cdm.link/beads-is-now-open-source/
- CDM, "A life cycle for open modules as Mutable Instruments joins VCV Rack" (May 2018): https://cdm.link/a-life-cycle-for-open-modules-as-mutable-instruments-joins-vcv-rack/
- CDM, "Free modular: Mutable Instruments in VCV Rack" (April 2020): https://cdm.link/2020/04/free-modular-mutable-instruments-vcv-rack/
- Perfect Circuit, "Mutable Instruments retrospective": https://www.perfectcircuit.com/signal/mutable-instruments-retrospective
- Synthtopia, "Eurorack losing one of its most influential companies" (April 2022): https://www.synthtopia.com/content/2022/04/30/eurorack-losing-one-of-its-most-influential-companies-mutable-instruments/
- Sonicstate, "Mutable Instruments successor to Clouds: Beads" (February 2021): https://sonicstate.com/news/2021/02/17/mutable-have-successor-to-clouds-beads
- Mutable Instruments documentation, Beads: https://pichenettes.github.io/mutable-instruments-documentation/modules/beads/
- pichenettes/eurorack README (licenses and naming guidelines): https://github.com/pichenettes/eurorack
- Tidal Cycles, mi-UGens reference: https://tidalcycles.org/docs/reference/mi-ugens
