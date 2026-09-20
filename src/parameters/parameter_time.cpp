/******************************************************************************
    Copyright (C) 2025 by xurei <xureilab@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "parameter_time.hpp"
#include "../effect.h"
#include "../shadertastic_common.hpp"

void effect_parameter_time::tick(shadertastic_common *s) {
    uint64_t frame_interval = obs_get_frame_interval_ns();
    (*time) += (
        // Converting to double first before reconverting to float to keep precision. Might be useless
        speed < 0.0001 ? 0.0f : (float)(((double)frame_interval/1000000000.0) * speed)
    );
    if (reset_on_show && !s->was_enabled) {
        *time = 0.0;
    }
}
