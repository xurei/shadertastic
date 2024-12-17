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

#include <thread>
#include "onnxmediapipe/models_provider.h"

namespace onnxmediapipe
{
    std::unique_ptr<Ort::Env> ort_env;

    std::shared_ptr<FaceDetection> ModelsProvider::faceDetection;
    std::shared_ptr<FaceLandmarks> ModelsProvider::faceLandmarks;

    void ModelsProvider::initialize() {
        if (!ort_env) {
            const char *instanceName = "shadertastic-onnx-inference";

            // TODO maybe add a global setting in OBS to allocate the number of threads ?
            Ort::ThreadingOptions ortThreadingOptions;
            ortThreadingOptions.SetGlobalInterOpNumThreads(
                std::min(
                    2, // No more than this many threads
                    std::max(1, (int) std::thread::hardware_concurrency() / 4) // A quarter of the threads that can concurrently run on the CPU
                )
            );
            ortThreadingOptions.SetGlobalIntraOpNumThreads(1);
            ort_env.reset(new Ort::Env(ortThreadingOptions, OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, instanceName));
        }
    }

    std::shared_ptr<FaceDetection> ModelsProvider::getFaceDetection() {
        if (faceDetection == nullptr) {
            faceDetection = std::make_shared<FaceDetection>(ort_env);
        }
        return faceDetection;
    }

    std::shared_ptr<FaceLandmarks> ModelsProvider::getFaceLandmarks() {
        if (faceLandmarks == nullptr) {
            faceLandmarks = std::make_shared<FaceLandmarks>(ort_env);
        }
        return faceLandmarks;
    }
}