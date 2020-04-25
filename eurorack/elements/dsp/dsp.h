// Copyright 2014 Emilie Gillet.
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

#ifndef ELEMENTS_DSP_DSP_H_
#define ELEMENTS_DSP_DSP_H_

#include "stmlib/stmlib.h"

#include <cmath>

namespace elements {
  
//static const float kSampleRate = 32000.0f;
const size_t kMaxBlockSize = 16;
    
    class Dsp {
    private:
        static float kSampleRate;
        static float kSrFactor;
        static float kIntervalCorrection;
        
    public:
        static float getSr() { return kSampleRate; }
        static float getSrFactor() { return kSrFactor; }
        static float getIntervalCorrection() { return kIntervalCorrection; }
        static void setSr(double newsr) {
            kSampleRate = newsr;
            kSrFactor = 32000.0f / kSampleRate;
            kIntervalCorrection = logf(kSrFactor)/logf(2.0f)*12.0f;
        }
    };

}  // namespace elements

#endif  // ELEMENTS_DSP_DSP_H_
