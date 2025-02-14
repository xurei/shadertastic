// Copyright(C) 2022-2023 Intel Corporation
// SPDX - License - Identifier: Apache - 2.0
#pragma once

#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>
#include "onnxmediapipe/common.h"
#include "../../../src/util/tuple.h"

namespace onnxmediapipe
{
    class FaceDetection;
    class FaceLandmarks;

    class FaceMesh
    {
    public:
        explicit FaceMesh();

        inline bool IsFaceDetectionNeeded() {
            return _bNeedsDetection;
        }

        float * getFaceDetectionInputTensorBuffer();

        float * getFaceMeshInputTensorBuffer();

        inline float2 getROICenter() {
            return float2{
                .x = _tracked_roi.center_x,
                .y = _tracked_roi.center_y,
            };
        }

        inline float2 getROISize() {
            return float2{
                .x = _tracked_roi.width,
                .y = _tracked_roi.height,
            };
        }

        inline float getROIRotation() {
            return _tracked_roi.rotation;
        }

        // Given a BGR frame, detects face.
        bool RunFaceDetection(const cv::Mat &imageBGR);

        // Given a BGR frame, generate landmarks.
        bool Run(const cv::Mat &imageBGR, int image_width, int image_height, FaceLandmarksResults& results);

        RotatedRect _tracked_roi = {};

    private:
        std::shared_ptr<FaceDetection> _facedetection;
        std::vector<DetectedObject> objects;
        std::shared_ptr<FaceLandmarks> _facelandmarks;

        bool _bNeedsDetection = true;

        //static ov::Core core;
    };

} //namespace onnxmediapipe
