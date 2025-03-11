/* -*- coding: utf-8 -*-
 *
 * Source: https://github.com/casiez/OneEuroFilter
 *
 * OneEuroFilter.cpp -
 *
 * Authors:
 * Nicolas Roussel (nicolas.roussel@inria.fr)
 * Géry Casiez https://gery.casiez.net
 *
 * Copyright 2019 Inria
 *
 * BSD License https://opensource.org/licenses/BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "one_euro_filter.h"
#include "../logging_functions.hpp"

// Math constants are not always defined.
#ifndef M_PI2
#define M_PI2 6.28318530718f
#endif
// -----------------------------------------------------------------

void LowPassFilter::setAlpha(float alpha_) {
    if (alpha_ <= 0.0 || alpha_ > 1.0) {
        #ifdef __EXCEPTIONS
            throw std::range_error("alpha should be in (0.0., 1.0] and its current value is " + std::to_string(alpha_));
        #else
            alpha_ = 0.5;
        #endif
    }
    this->alpha = alpha_;
}

LowPassFilter::LowPassFilter(float alpha, float initval) {
    _lastRawValue = _lastFilteredValue = initval;
    setAlpha(alpha);
    initialized = false;
}

float LowPassFilter::filterWithAlpha(float value, float alpha_) {
    float result;
    if (initialized) {
        result = alpha_ * value + (1.0f - alpha_) * _lastFilteredValue;
    }
    else {
        result = value;
        initialized = true;
    }
    _lastRawValue = value;
    _lastFilteredValue = result;
    return result;
}

bool LowPassFilter::hasLastRawValue() const {
    return initialized;
}

float LowPassFilter::lastRawValue() const {
    return _lastRawValue;
}

float LowPassFilter::lastFilteredValue() const {
    return _lastFilteredValue;
}
// ---------------------------------------------------------------------------------------------------------------------

OneEuroFilter::OneEuroFilter(float freq, float mincutoff, float beta_, float dcutoff) :
    x(alpha(mincutoff)), dx(alpha(mincutoff)) {
    setFrequency(freq);
    setMinCutoff(mincutoff);
    setBeta(beta_);
    setDerivateCutoff(dcutoff);
}

float OneEuroFilter::alpha(float cutoff) const {
//    float te = 1.0f / freq;
//    float tau = 1.0f / (M_PI2 * cutoff);
//    return 1.0f / (1.0f + tau / te);
    float tau = 1.0f / (M_PI2 * cutoff);
    return 1.0f / (1.0f + tau * freq);
}

void OneEuroFilter::setFrequency(float f) {
    if (f <= 0) {
        #ifdef __EXCEPTIONS
            throw std::range_error("freq should be >0");
        #else
            f= 120 ;  // set to 120Hz default
        #endif
    }

    freq = f;
}

void OneEuroFilter::setMinCutoff(float mc) {
    if (mc <= 0) {
        #ifdef __EXCEPTIONS
            throw std::range_error("mincutoff should be >0");
        #else
            mc = 1.0;
        #endif
    }
    mincutoff = mc;
}

void OneEuroFilter::setBeta(float b) {
    beta_ = b;
}

void OneEuroFilter::setDerivateCutoff(float dc) {
    if (dc <= 0) {
        #ifdef __EXCEPTIONS
            throw std::range_error("dcutoff should be >0");
        #else
            dc = 1.0;
        #endif
    }
    dcutoff = dc;
}

float OneEuroFilter::filter(float value, TimeStamp deltatime, bool do_debug) {
    // update the sampling frequency based on timestamps
    if (deltatime != UndefinedTime && deltatime > 0.0f) {
        freq = 1.0f / deltatime;
    }
    // estimate the current variation per second
    // Fixed in 08/23 to use lastFilteredValue
    float dvalue = x.hasLastRawValue() ? (value - x.lastFilteredValue()) * freq : 0.0f; // FIXME: 0.0 or value?

    float edvalue = dx.filterWithAlpha(dvalue, alpha(dcutoff));
    // use it to update the cutoff frequency
    float cutoff = mincutoff + beta_ * fabsf(edvalue);

    float alpha_cutoff = alpha(cutoff);

    if (do_debug) {
        debug("dvalue=%f   edvalue=%f   cutoff=%f   alpha(cutoff)=%f", dvalue, edvalue, cutoff, alpha_cutoff);
    }

    // filter the given value
    return x.filterWithAlpha(value, alpha_cutoff);
}
