// Copyright(C) 2022-2023 Intel Corporation
// SPDX - License - Identifier: Apache - 2.0
#ifndef ONNX_FACE_LANDMARKS_H
#define ONNX_FACE_LANDMARKS_H

#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>
#include <onnxruntime_cxx_api.h>
#include "onnxmediapipe/common.h"

namespace onnxmediapipe
{
    class FaceLandmarks {
    public:
        explicit FaceLandmarks(std::unique_ptr<Ort::Env> &ort_env);

        void Run(const cv::Mat& frameBGR, int image_width, int image_height, const RotatedRect& roi, FaceLandmarksResults& results);

        float * getInputTensorBuffer() {
            return inputTensorValues[0].data();
        }

    private:
        void preprocess(const cv::Mat& frameBGR);
        void postprocess(int image_width, int image_height, const RotatedRect& roi, FaceLandmarksResults& results);

        std::unique_ptr<Ort::Session> ortSession;
        size_t netInputHeight = 0;
        size_t netInputWidth = 0;

        size_t inputCount;
        size_t outputCount;
        std::vector<const char *> inputNames;
        std::vector<const char *> outputNames;

        std::vector<std::vector<float>> inputTensorValues;
        std::vector<std::vector<float>> outputTensorValues;
        std::vector<Ort::Value> inputTensors;
        std::vector<Ort::Value> outputTensors;

//        ov::CompiledModel compiledModel;
//        ov::InferRequest inferRequest;

        bool _bWithAttention = true;

        void refinePoints(FaceLandmarksResults &results, float dx, float dy, std::vector<int> &indices, float reduction);
    };

} //ovfacemesh

#endif /* ONNX_FACE_LANDMARKS_H */
