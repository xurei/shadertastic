// Copyright(C) 2022-2023 Intel Corporation
// SPDX - License - Identifier: Apache - 2.0
#pragma once

#include <array>
#include <numeric>
#include <opencv2/core.hpp>
#include <vector>
#include "onnxmediapipe/landmark_refinement_indices.h"

#ifndef M_PI
# define M_PI		3.14159265358979323846	/* pi */
#endif

namespace onnxmediapipe
{
    struct RotatedRect {
        float center_x;
        float center_y;
        float width;
        float height;
        float rotation;
    };

    std::string printShape(const std::vector<int64_t>& v);
    const char * printElementType(const ONNXTensorElementDataType& type);
    void hwcToChw(cv::InputArray src, cv::OutputArray dst);

    template <typename T> T vectorProduct(const std::vector<T>& v) {
        return std::accumulate(v.begin(), v.end(), static_cast<T>(1l), std::multiplies<T>());
    }

    void GetRotatedSubRectToRectTransformMatrix(const RotatedRect& sub_rect,
        int rect_width, int rect_height,
        bool flip_horizontaly,
        std::array<float, 16>* matrix_ptr);

    struct FaceLandmarksResults {
        //468 3d landmarks
        // x- and y-coordiates follow the image pixel coordinates;
        // z-coordinates are relative to the face center of mass and are
        // scaled proportionally to the face width
        cv::Point3f facial_surface[facial_surface_num_points];

        //80 2d landmarks (inner and outer contours and an intermediate line)
        //(only populated if using 'with attention' model)
        cv::Point2f lips_refined_region[lips_refined_region_num_points];

        //71 2d landmarks (inner and outer contours and an intermediate line)
        //(only populated if using 'with attention' model)
        cv::Point2f left_eye_refined_region[eye_refined_region_num_points];

        //71 2d landmarks (inner and outer contours and an intermediate line)
        //(only populated if using 'with attention' model)
        cv::Point2f right_eye_refined_region[eye_refined_region_num_points];

        //5 2d landmarks (1 for pupil center and 4 for iris contour)
        //(only populated if using 'with attention' model)
        cv::Point2f left_iris_refined_region[iris_refined_region_num_points];

        //5 2d landmarks (1 for pupil center and 4 for iris contour)
        //(only populated if using 'with attention' model)
        cv::Point2f right_iris_refined_region[iris_refined_region_num_points];

        //Refined landmarks
        // If using the 'with attention' model, this will be 478 points
        //  generated from the 6 'raw' landmark regions above.
        // If *not* using the 'with attention' model, this will simply be
        //  the same exact 468 point list as 'facial_surface'.
        cv::Point3f refined_landmarks[refined_landmarks_num_points];

        //indicates the likelihood of the face being present in the input image.
        float face_flag;

        //'tracked' ROI. This is what you would use to pass into the next 'Run',
        //  depending of course on 'face_flag'.
        RotatedRect roi;
    };

    struct PoseLandmarkResult {
        //todo: change to 'PoseLandmark'
        struct PoseLandmarkKeypoint {
            cv::Point3f coord;
            float visibility;
            float presence;
        };

        //todo: change to 'landmarks'
        std::vector< PoseLandmarkKeypoint > keypoints;

        //indicates the likelihood of the face being present in the input image.
        float pose_flag;

        //TODO: seg mask?

        //'tracked' ROI. This is what you would use to pass into the next 'Run',
        //  depending of course on 'face_flag'.
        RotatedRect roi;
    };

//    static inline ov::Layout getLayoutFromShape(const ov::Shape& shape) {
//        if (shape.size() == 2) {
//            return "NC";
//        }
//        else if (shape.size() == 3) {
//            return (shape[0] >= 1 && shape[0] <= 4) ? "CHW" :
//                "HWC";
//        }
//        else if (shape.size() == 4) {
//            return (shape[1] >= 1 && shape[1] <= 4) ? "NCHW" :
//                "NHWC";
//        }
//        else {
//            throw std::runtime_error("Usupported " + std::to_string(shape.size()) + "D shape");
//        }
//    }

//    static inline void logBasicModelInfo(const std::shared_ptr<ov::Model>& model) {
//        std::cout << "Model name: " << model->get_friendly_name() << std::endl;
//
//        // Dump information about model inputs/outputs
//        ov::OutputVector inputs = model->inputs();
//        ov::OutputVector outputs = model->outputs();
//
//        std::cout << "\tInputs: " << std::endl;
//        for (const ov::Output<ov::Node>& input : inputs) {
//            const std::string name = input.get_any_name();
//            const ov::element::Type type = input.get_element_type();
//            const ov::PartialShape shape = input.get_partial_shape();
//            const ov::Layout layout = ov::layout::get_layout(input);
//
//            std::cout << "\t\t" << name << ", " << type << ", " << shape << ", " << layout.to_string() << std::endl;
//        }
//
//        std::cout << "\tOutputs: " << std::endl;
//        for (const ov::Output<ov::Node>& output : outputs) {
//            const std::string name = output.get_any_name();
//            const ov::element::Type type = output.get_element_type();
//            const ov::PartialShape shape = output.get_partial_shape();
//            const ov::Layout layout = ov::layout::get_layout(output);
//
//            std::cout << "\t\t" << name << ", " << type << ", " << shape << ", " << layout.to_string() << std::endl;
//        }
//
//        return;
//    }

    static inline float NormalizeRadians(float angle) {
        return (float)(angle - 2 * M_PI * std::floor((angle - (-M_PI)) / (2 * M_PI)));
    }

    //calculate overlap similarity using Intersection Over Union
    static inline float OverlapSimilarity(const cv::Rect2f& rect1, const cv::Rect2f& rect2) {
        cv::Rect2f intersection = rect1 & rect2;
        const float intersection_area = intersection.area();
        if (intersection_area == 0.f)
            return 0.f;

        float normalization;
        normalization = rect1.area() + rect2.area() - intersection_area;

        return normalization > 0.0f ? intersection_area / normalization : 0.0f;
    }

    struct DetectedObject : public cv::Rect2f {
        unsigned int labelID = 0;
        std::string label = "";
        float confidence = 0.f;
        float rotation = 0.f;

        bool bCenterValid = false;
        cv::Point2f center;

        std::vector<cv::Point2f> keypoints;
    };


}
