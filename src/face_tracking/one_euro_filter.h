/* -*- coding: utf-8 -*-
 *
 * Source: https://github.com/casiez/OneEuroFilter
 *
 * OneEuroFilter.h -
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

#include <iostream>
#include <stdexcept>
#include <cmath>
#include <ctime>
// -----------------------------------------------------------------
// Utilities

typedef float TimeStamp; // in seconds

static const TimeStamp UndefinedTime = -1.0;
// -----------------------------------------------------------------

class LowPassFilter {
private:
    float _lastRawValue{}, alpha = 0.5, _lastFilteredValue{};
    bool initialized{};

public:
    explicit LowPassFilter(float alpha, float initval = 0.0);

    void setAlpha(float alpha);

    float filterWithAlpha(float value, float alpha);

    inline float filter(float value) {
        return filterWithAlpha(value, alpha);
    }

    [[nodiscard]] bool hasLastRawValue() const;

    [[nodiscard]] float lastRawValue() const;

    [[nodiscard]] float lastFilteredValue() const;
};
// -----------------------------------------------------------------

class OneEuroFilter {
private:
    float freq{};
    float mincutoff{};
    float beta_{};
    float dcutoff{};
    LowPassFilter x;
    LowPassFilter dx;

    [[nodiscard]] float alpha(float cutoff) const;

public:
    /**
     * @brief Creates the filter and set its parameters
     * @param freq An estimate of the frequency in Hz of the signal (> 0), if timestamps are not available.
     * @param mincutoff Min cutoff frequency in Hz (> 0). Lower values allow to remove more jitter.
     * @param beta_ Parameter to reduce latency (> 0).
     * @param dcutoff Used to filter the derivates. 1 Hz by default. Change this parameter if you know what you are doing.
     */
    explicit OneEuroFilter(float freq, float mincutoff = 1.0, float beta_ = 0.0, float dcutoff = 1.0);

    /**
     * @brief Filter the noisy signal
     * @param value Noisy value to filter
     * @param timestamp (optional) timestamp in seconds
     * @return The filtered value
     */
    float filter(float value, TimeStamp timestamp = UndefinedTime, bool do_debug = false);

    /**
     * @brief Sets the frequency of the signal
     * @param f An estimate of the frequency in Hz of the signal (> 0), if timestamps are not available.
     */
    void setFrequency(float f);

    /**
     * @brief Sets the filter min cutoff frequency
     * @param mc Min cutoff frequency in Hz (> 0). Lower values allow to remove more jitter.
     */
    void setMinCutoff(float mc);

    /**
     * @brief Sets the Beta parameter
     * @param b Parameter to reduce latency (> 0).
     */
    void setBeta(float b);

    /**
     * @brief Sets the Cutoff frequency for derivates
     * @param dc Used to filter the derivates. 1 Hz by default. Change this parameter if you know what you are doing.
     */
    void setDerivateCutoff(float dc);
};
