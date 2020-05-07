// Copyright 2015 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Utility DSP routines.

#ifndef RINGS_DSP_DSP_H_
#define RINGS_DSP_DSP_H_

#include "stmlib/stmlib.h"

// #define MIC_W
#define BRYAN_CHORDS

namespace rings {
  
  //  static const float kSampleRate = 44100.f; //48000.0f;
//const float a3 = 440.0f / kSampleRate;
    
    // TODO: changing this will affect the way the unit reacts
    // e.g. damping in easteregg mode etc.
    const size_t kMaxBlockSize = 128;     //24;
    
    // add some code to make SR settable
    class Dsp {
        private:
        static float sr;
        static float a3;
        //static size_t maxBlockSize;
        
        public:
        static float getSr() {return sr;}
        static float getA3() {return a3;}
        static void setSr(float newsr) {
            sr = newsr;
            a3 = 440.0f / sr;
        }
        /*
        static size_t getBlockSize() {return maxBlockSize;}
        static void setBlockSize(int newBlockSize) {
            maxBlockSize = static_cast<size_t>(newBlockSize);
        }*/
    };

}  // namespace rings

#endif  // RINGS_DSP_DSP_H_
