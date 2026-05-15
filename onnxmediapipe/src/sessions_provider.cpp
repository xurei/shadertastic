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
#include "onnxmediapipe/sessions_provider.h"
#include <obs-module.h>

namespace onnxmediapipe
{
    std::unique_ptr<Ort::Session> SessionsProvider::initializeSession(std::unique_ptr<Ort::Env> &ort_env, const char *model_data_path) {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        sessionOptions.EnableCpuMemArena();
        sessionOptions.EnableMemPattern();
        sessionOptions.DisablePerSessionThreads();
        sessionOptions.SetExecutionMode(ExecutionMode::ORT_PARALLEL);

        // TODO sessionOptions.AppendExecutionProvider_DML

        blog(LOG_INFO, "Using ONNX Runtime CPU provider");

        #if defined(_WIN32)
            std::string model_data_path_ = std::string(model_data_path);
            std::wstring model_data_path__ = std::wstring(model_data_path_.begin(), model_data_path_.end());
            return std::make_unique<Ort::Session>(*ort_env, (const ORTCHAR_T*)(model_data_path__.c_str()), sessionOptions);
        #else
            return std::make_unique<Ort::Session>(*ort_env, model_data_path, sessionOptions);
        #endif
    }
}