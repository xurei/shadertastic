/******************************************************************************
    Copyright (C) 2024 by xurei <xureilab@gmail.com>

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

#ifndef ONNX_SESSIONS_PROVIDER_H
#define ONNX_SESSIONS_PROVIDER_H

#include <memory>
#include "onnxmediapipe/face_detection.h"
#include "onnxmediapipe/face_landmarks.h"

namespace onnxmediapipe
{
    class SessionsProvider {
    public:
        static std::unique_ptr<Ort::Session> initializeSession(std::unique_ptr<Ort::Env> &ort_env, const char *model_data_path);
    };
}

#endif /* ONNX_SESSIONS_PROVIDER_H */