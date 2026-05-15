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

#include <vector>
#include <iostream>
#include <opencv2/core.hpp>
#include <obs-module.h>
#include "onnxmediapipe/face_landmarks.h"
#include "onnxmediapipe/sessions_provider.h"
#include "onnxmediapipe/landmark_refinement_indices.h"

#include "../../src/logging_functions.hpp"
#include "../../src/fdebug.h"
#include "../../src/util/time_util.hpp"

FILE* faceLandmarksDebugFile;

namespace onnxmediapipe
{
    const Ort::RunOptions runOptions{nullptr};

    FaceLandmarks::FaceLandmarks(std::unique_ptr<Ort::Env> &ort_env) {
        unsigned long tic = get_time_ms();
        if (!ortSession) {
            char *model_data_path = obs_module_file("face_detection_models/face_landmark_with_attention_192x192.onnx");

            if (model_data_path) {
                ortSession = SessionsProvider::initializeSession(ort_env, model_data_path);
                bfree(model_data_path);
                debug("FACE_LANDMARKS Loading model %s", model_data_path);
            }
        }
        if (!ortSession) {
            return;
        }

        // Prepare inputs / outputs
        {
            if (ortSession->GetInputCount() != 1) {
                throw std::logic_error("FaceDetection model topology should have only 1 input");
            }

            inputCount = ortSession->GetInputCount();
            outputCount = ortSession->GetOutputCount();
            debug("FACE_LANDMARKS Inputs: %lu", inputCount);
            debug("FACE_LANDMARKS Outputs: %lu", outputCount);

            Ort::AllocatorWithDefaultOptions allocator;
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

            std::vector<std::vector<int64_t>> inputShapes(inputCount);
            for (size_t i = 0; i < inputCount; ++i) {
                std::string inputName = ortSession->GetInputNameAllocated(i, allocator).get();
                char *inputNameCStr = new char[inputName.length()+1];
                strcpy(inputNameCStr, inputName.c_str());
                inputNames.push_back(inputNameCStr);
                debug("FACE_LANDMARKS Input Name %lu: %s", i, inputNameCStr);
                std::vector<int64_t> inputShape = ortSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
                if (inputShape[0] < 0) {
                    //Negative batch size, aka dynamic. We'll only use one
                    inputShape[0] = 1;
                }
                auto elementType = ortSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetElementType();
                inputShapes[i] = inputShape;
                debug("FACE_LANDMARKS Input Shape %lu: %s", i, printShape(inputShape).c_str());
                debug("FACE_LANDMARKS Input Type %lu: %s", i, printElementType(elementType));
                int64_t inputTensorSize = vectorProduct(inputShape);
                inputTensorValues.push_back(std::vector<float>((size_t)inputTensorSize));

                inputTensors.push_back(Ort::Value::CreateTensor<float>(
                    memoryInfo,
                    inputTensorValues[i].data(), (size_t)inputTensorSize,
                    inputShape.data(), inputShape.size()
                ));
            }

            std::vector<std::vector<int64_t>> outputShapes(outputCount);
            for (size_t i = 0; i < outputCount; ++i) {
                std::string outputName = ortSession->GetOutputNameAllocated(i, allocator).get();
                char *outputNameCStr = new char[outputName.length()+1];
                strcpy(outputNameCStr, outputName.c_str());
                outputNames.push_back(outputNameCStr);
                debug("FACE_LANDMARKS Output Name %lu: %s", i, outputNameCStr);
                std::vector<int64_t> outputShape = ortSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
                if (outputShape[0] < 0) {
                    //Negative batch size, aka dynamic. We'll only use one
                    outputShape[0] = 1;
                }
                outputShapes[i] = outputShape;
                debug("FACE_LANDMARKS Output Shape %lu: %s", i, printShape(outputShape).c_str());
                size_t outputTensorSize = vectorProduct(outputShape);
                outputTensorValues.push_back(std::vector<float>(outputTensorSize));

                outputTensors.push_back(Ort::Value::CreateTensor<float>(
                    memoryInfo,
                    outputTensorValues[i].data(), outputTensorSize,
                    outputShape.data(), outputShape.size()
                ));
            }
            netInputHeight = inputShapes[0][2];
            netInputWidth = inputShapes[0][3];
        }

        unsigned long toc = get_time_ms();
        debug("FACE_LANDMARKS Done Loading model in %li ms", toc-tic);
    }

    void FaceLandmarks::Run(const cv::Mat& frameBGR, int image_width, int image_height, const RotatedRect& roi, FaceLandmarksResults& results) {
        #ifdef DEV_MODE
                unsigned long tic = get_time_us();
        #endif
        faceLandmarksDebugFile = fdebug_open("face_landmarks.txt");
        preprocess(frameBGR);
        debug_trace("  a %lu", get_time_us()-tic);

        // Perform inference
        /* To run inference, we provide the run options, an array of input names corresponding to the
        inputs in the input tensor, an array of input tensor, number of inputs, an array of output names
        corresponding to the outputs in the output tensor, an array of output tensor, number of outputs. */
        ortSession->Run(runOptions,
            inputNames.data(), inputTensors.data(), inputCount,
            outputNames.data(), outputTensors.data(), outputCount);
        debug_trace("  b %lu", get_time_us()-tic);

        // post-process
        postprocess(image_width, image_height, roi, results);
        fdebug_close(faceLandmarksDebugFile);
        debug_trace("  c %lu", get_time_us()-tic);
    }

    void FaceLandmarks::preprocess(const cv::Mat &frameBGR) {
        // Wrap the already-allocated tensor as a cv::Mat of floats
        float* pTensor = inputTensorValues[0].data();
        cv::Mat converted = cv::Mat((int)netInputHeight*3, (int)netInputWidth, CV_32FC1, pTensor);

        //hwcToChw
        std::vector<cv::Mat> channels;
        cv::split(frameBGR, channels);
        // Concatenate three vectors to one
        cv::vconcat(channels, converted);
    }

    static inline void fill2d_points_results(const float* raw_tensor, const size_t num_points, cv::Point2f v[], const int netInputWidth, const int netInputHeight) {
        const float inv_net_width = 1.0f / (float)netInputWidth;
        const float inv_net_height = 1.0f / (float)netInputHeight;
        for (size_t i = 0; i < num_points; i++) {
            v[i].x = raw_tensor[i * 2] * inv_net_width;
            v[i].y = raw_tensor[i * 2 + 1] * inv_net_height;
        }
    }

    static inline void transform_point(cv::Point2f &p, const float cos_angle, const float sin_angle, const RotatedRect &rect) {
        const float x = p.x - 0.5f;
        const float y = p.y - 0.5f;
        const float new_x = cos_angle * x - sin_angle * y;
        const float new_y = sin_angle * x + cos_angle * y;

        p.x = new_x * rect.width + rect.center_x;
        p.y = new_y * rect.height + rect.center_y;
    }

    void FaceLandmarks::refineChinPoints(FaceLandmarksResults& results, float dx, float dy, std::vector<int> &sublipIndices, float reduction) {
        for(int idx : sublipIndices) {
            dx *= reduction;
            dy *= reduction;
            results.refined_landmarks[idx].x += dx;
            results.refined_landmarks[idx].y += dy;
        }
    }

    void FaceLandmarks::postprocess(int image_width, int image_height, const RotatedRect& roi, FaceLandmarksResults& results) {
        results.face_flag = 0.f;

        const float* facial_surface_tensor_data = outputTensorValues[4].data();
        const float* face_flag_data = outputTensorValues[0].data();

//        //double check that the output tensors have the correct size
//        {
//            size_t facial_surface_tensor_size = inferRequest.get_tensor(facial_surface_tensor_name).get_byte_size();
//            if (facial_surface_tensor_size < (nFacialSurfaceLandmarks * 3 * sizeof(float)))
//            {
//                throw std::logic_error("facial surface tensor is holding a smaller amount of data than expected.");
//            }
//
//            size_t face_flag_tensor_size = inferRequest.get_tensor(face_flag_tensor_name).get_byte_size();
//            if (face_flag_tensor_size < (sizeof(float)))
//            {
//                throw std::logic_error("face flag tensor is holding a smaller amount of data than expected.");
//            }
//        }

        //apply sigmoid activation to produce face flag result
        results.face_flag = 1.0f / (1.0f + std::exp(-(*face_flag_data)));
        fdebug(faceLandmarksDebugFile, "Face Flag: %f → %f", *face_flag_data, results.face_flag);

        // Pre-compute common values
        const float inv_net_width = 1.0f / (float)netInputWidth;
        const float inv_net_height = 1.0f / (float)netInputHeight;

        // Process facial surface points - optimized for cache locality
        for (size_t i = 0; i < facial_surface_num_points; i++) {
            const size_t idx = i * 3;
            results.facial_surface[i].x = facial_surface_tensor_data[idx] * inv_net_width;
            results.facial_surface[i].y = facial_surface_tensor_data[idx + 1] * inv_net_height;
            results.facial_surface[i].z = facial_surface_tensor_data[idx + 2] * inv_net_width;
            //fdebug(faceLandmarksDebugFile, "%i %f %f %f", i, results.facial_surface[i].x, results.facial_surface[i].y, results.facial_surface[i].z);
        }

        if (_bWithAttention) {
            const float* lips_refined_region_data = outputTensorValues[3].data();
            const float* left_eye_refined_region_data = outputTensorValues[1].data();
            const float* right_eye_refined_region_data = outputTensorValues[5].data();
            const float* left_iris_refined_region_data = outputTensorValues[2].data();
            const float* right_iris_refined_region_data = outputTensorValues[6].data();

//            //double check that the output tensors have the correct size
//            {
//                if (inferRequest.get_tensor(lips_refined_tensor_name).get_byte_size() < lips_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(lips_refined_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(left_eye_with_eyebrow_tensor_name).get_byte_size() < left_eye_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(left_eye_with_eyebrow_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(right_eye_with_eyebrow_tensor_name).get_byte_size() < right_eye_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(right_eye_with_eyebrow_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(left_iris_refined_tensor_name).get_byte_size() < left_iris_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(left_iris_refined_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(right_iris_refined_tensor_name).get_byte_size() < right_iris_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(right_iris_refined_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//            }

            fill2d_points_results(lips_refined_region_data,       lips_refined_region_num_points, results.lips_refined_region,       (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(left_eye_refined_region_data,   eye_refined_region_num_points,  results.left_eye_refined_region,   (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(right_eye_refined_region_data,  eye_refined_region_num_points,  results.right_eye_refined_region,  (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(left_iris_refined_region_data,  iris_refined_region_num_points, results.left_iris_refined_region,  (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(right_iris_refined_region_data, iris_refined_region_num_points, results.right_iris_refined_region, (int)netInputWidth, (int)netInputHeight);

            //create a (normalized) refined list of landmarks from the 6 separate lists that we generated.

            //initialize the first 468 points to our face surface landmarks
            memcpy(results.refined_landmarks, results.facial_surface, facial_surface_num_points * sizeof(cv::Point3f));

            //override x & y for lip points
            for (size_t i = 0; i < lips_refined_region_num_points; i++) {
                const size_t idx = lips_refinement_indices[i];
                float dx = results.lips_refined_region[i].x - results.refined_landmarks[idx].x;
                float dy = results.lips_refined_region[i].y - results.refined_landmarks[idx].y;
                results.refined_landmarks[idx].x += dx;
                results.refined_landmarks[idx].y += dy;

                switch (idx) {
                    case 17: {
                        static std::vector<int> sublipIndices = {18, 200, 199, 175, 152};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.99f);
                        break;
                    }
                    case 84: {
                        static std::vector<int> sublipIndices = {83, 201, 208, 171, 148};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.98f);
                        break;
                    }
                    case 314: {
                        static std::vector<int> sublipIndices = {313, 421, 428, 396, 377};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.98f);
                        break;
                    }
                    case 405: {
                        static std::vector<int> sublipIndices = {406, 418, 262, 369, 400};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.97f);
                        break;
                    }
                    case 181: {
                        static std::vector<int> sublipIndices = {182, 194, 32, 140, 176};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.97f);
                        break;
                    }
                    case 91: {
                        static std::vector<int> sublipIndices = {106, 204, 211, 170, 149};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.96f);
                        break;
                    }
                    case 321: {
                        static std::vector<int> sublipIndices = {335, 424, 431, 395, 378};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.96f);
                        break;
                    }
                    case 375: {
                        static std::vector<int> sublipIndices = {273, 422, 430, 394, 379};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.95f);
                        break;
                    }
                    case 146: {
                        static std::vector<int> sublipIndices = {43, 202, 210, 169, 150};
                        refineChinPoints(results, dx, dy, sublipIndices, 0.95f);
                        break;
                    }
                    default: break;
                }
            }

            //override x & y for left & right_eye points
            for (size_t i = 0; i < eye_refined_region_num_points; i++) {
                const size_t right_idx = right_eye_refinement_indices[i];
                const size_t left_idx = left_eye_refinement_indices[i];

                results.refined_landmarks[right_idx].x = results.right_eye_refined_region[i].x;
                results.refined_landmarks[right_idx].y = results.right_eye_refined_region[i].y;

                results.refined_landmarks[left_idx].x = results.left_eye_refined_region[i].x;
                results.refined_landmarks[left_idx].y = results.left_eye_refined_region[i].y;
            }

            float z_avg_for_left_iris = 0.f;
            float z_avg_for_right_iris = 0.f;
            for (int i = 0; i < 16; i++) {
                z_avg_for_left_iris += results.refined_landmarks[left_iris_z_avg_indices[i]].z;
                z_avg_for_right_iris += results.refined_landmarks[right_iris_z_avg_indices[i]].z;
            }
            z_avg_for_left_iris *= 0.0625f;  // 0.0625 == 1.0/16.0
            z_avg_for_right_iris *= 0.0625f;

            //set x & y for left & right iris points
            for (size_t i = 0; i < iris_refined_region_num_points; i++) {
                const size_t left_idx = left_iris_refinement_indices[i];
                const size_t right_idx = right_iris_refinement_indices[i];

                results.refined_landmarks[left_idx].x = results.left_iris_refined_region[i].x;
                results.refined_landmarks[left_idx].y = results.left_iris_refined_region[i].y;
                results.refined_landmarks[left_idx].z = z_avg_for_left_iris;

                results.refined_landmarks[right_idx].x = results.right_iris_refined_region[i].x;
                results.refined_landmarks[right_idx].y = results.right_iris_refined_region[i].y;
                results.refined_landmarks[right_idx].z = z_avg_for_right_iris;
            }
        }

        //project the points back into the pre-rotated / pre-cropped space
        {
            const RotatedRect normalized_rect = roi;

            const float angle = normalized_rect.rotation;
            const float sin_angle = std::sin(angle);
            const float cos_angle = std::cos(angle);

            for (size_t i = 0; i < refined_landmarks_num_points; i++) {
                cv::Point3f &p = results.refined_landmarks[i];
                const float x = p.x - 0.5f;
                const float y = p.y - 0.5f;
                const float new_x = cos_angle * x - sin_angle * y;
                const float new_y = sin_angle * x + cos_angle * y;

                p.x = new_x * normalized_rect.width + normalized_rect.center_x;
                p.y = new_y * normalized_rect.height + normalized_rect.center_y;
                p.z = p.z * normalized_rect.width;  // Scale Z coordinate as X.
            }

            for (auto& p : results.lips_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.left_eye_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.right_eye_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.left_iris_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.right_iris_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }
        }

        //from the refined landmarks, generated the RotatedRect to return.
        {
            float x_min = std::numeric_limits<float>::max();
            float x_max = std::numeric_limits<float>::min();
            float y_min = std::numeric_limits<float>::max();
            float y_max = std::numeric_limits<float>::min();

            for (const auto &p : results.refined_landmarks) {
                x_min = std::min(x_min, p.x);
                x_max = std::max(x_max, p.x);
                y_min = std::min(y_min, p.y);
                y_max = std::max(y_max, p.y);
            }

            float bbox_x = x_min;
            float bbox_y = y_min;
            const float bbox_width = x_max - x_min;
            const float bbox_height = y_max - y_min;

            results.roi.center_x = bbox_x + bbox_width * 0.5f;
            results.roi.center_y = bbox_y + bbox_height * 0.5f;
            results.roi.width = bbox_width;
            results.roi.height = bbox_height;

            //calculate rotation from keypoints 33 & 263
            const float x0 = results.refined_landmarks[33].x * (float)image_width;
            const float y0 = results.refined_landmarks[33].y * (float)image_height;
            const float x1 = results.refined_landmarks[263].x * (float)image_width;
            const float y1 = results.refined_landmarks[263].y * (float)image_height;

            results.roi.rotation = NormalizeRadians(-std::atan2(-(y1 - y0), x1 - x0));

            //final transform
            {
                const float image_width_f = (float)image_width;
                const float image_height_f = (float)image_height;

                float width = results.roi.width;
                float height = results.roi.height;
                const float rotation = results.roi.rotation;

                const float shift_x = 0.f;
                const float shift_y = 0.f;
                const float scale_x = 1.5f;
                const float scale_y = 1.5f;

                if (rotation == 0.f) {
                    results.roi.center_x = results.roi.center_x + width * shift_x;
                    results.roi.center_y = results.roi.center_y + height * shift_y;
                }
                else {
                    const float x_shift =
                        (image_width_f * width * shift_x * std::cos(rotation) -
                            image_height_f * height * shift_y * std::sin(rotation)) /
                        image_width_f;
                    const float y_shift =
                        (image_width_f * width * shift_x * std::sin(rotation) +
                            image_height_f * height * shift_y * std::cos(rotation)) /
                        image_height_f;

                    results.roi.center_x = results.roi.center_x + x_shift;
                    results.roi.center_y = results.roi.center_y + y_shift;
                }

                const float long_side = std::max(width * image_width_f, height * image_height_f);
                width = long_side / image_width_f;
                height = long_side / image_height_f;

                results.roi.width = width * scale_x;
                results.roi.height = height * scale_y;
            }
        }

        results.roi = results.roi;
    }
} //namespace onnxmediapipe
