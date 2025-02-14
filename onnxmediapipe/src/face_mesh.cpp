/******************************************************************************
    Copyright (C) 2023 by xurei <xureilab@gmail.com>

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

// NOTE : this file has been taken from https://github.com/intel/openvino-plugins-for-obs-studio and modified to use ONNX instead
// It has been widely modified since then

#include <iostream>
#include "onnxmediapipe/models_provider.h"
#include "onnxmediapipe/face_mesh.h"
#include "../../src/logging_functions.hpp"

namespace onnxmediapipe
{
    FaceMesh::FaceMesh():
        _facedetection(onnxmediapipe::ModelsProvider::getFaceDetection()),
        _facelandmarks(onnxmediapipe::ModelsProvider::getFaceLandmarks()) {}

    float * FaceMesh::getFaceDetectionInputTensorBuffer() {
        return _facedetection->getInputTensorBuffer();
    }

    float * FaceMesh::getFaceMeshInputTensorBuffer() {
        return _facelandmarks->getInputTensorBuffer();
    }

    bool FaceMesh::RunFaceDetection(const cv::Mat &frameBGR) {
        try {
            if (IsFaceDetectionNeeded()) {
                objects.clear();
                _facedetection->Run(frameBGR, objects);

                if (objects.empty()) {
                    return false;
                }

                _tracked_roi = {
                    .center_x = objects[0].center.x,
                    .center_y = objects[0].center.y,
                    .width = objects[0].width,
                    .height = objects[0].height,
                    .rotation = objects[0].rotation,
                };
            }
            return true;
        }
        catch (const Ort::Exception& e) {
            std::cerr << "Caught Ort::Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool FaceMesh::Run(const cv::Mat& imageBGR, int image_width, int image_height, FaceLandmarksResults& results) {
        try {
            _facelandmarks->Run(imageBGR, image_width, image_height, _tracked_roi, results);

            _tracked_roi = results.roi;

            _bNeedsDetection = (results.face_flag < 0.5f);

            return !_bNeedsDetection;
        }
        catch (const Ort::Exception& e) {
            std::cerr << "Caught Ort::Exception: " << e.what() << std::endl;
            return false;
        }
    }
} //namespace onnxmediapipe
