#ifndef SHADERTASTIC_RGBA_TO_RGB_H
#define SHADERTASTIC_RGBA_TO_RGB_H

#include <opencv2/core.hpp>

inline cv::Mat rgbaToRgb(const cv::Mat& rgbaImage) {
    // Split the RGBA image into individual channels
    std::vector<cv::Mat> channels;
    cv::split(rgbaImage, channels);

    // Extract the RGB channels (excluding the alpha channel)
    cv::Mat rgbImage;
    cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, rgbImage);

    return rgbImage;
}

#endif /* SHADERTASTIC_RGBA_TO_RGB_H */
