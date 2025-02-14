#ifndef SHADERTASTIC_RGBA_TO_RGB_H
#define SHADERTASTIC_RGBA_TO_RGB_H

#include <opencv2/core.hpp>

inline cv::Mat _rgbaToRgb(const cv::Mat& rgbaImage, int opencv_image_format) {
    // Create an output Mat with the same size but only 3 channels (RGB)
    cv::Mat rgbImage(rgbaImage.rows, rgbaImage.cols, opencv_image_format);

    // Use mixChannels to copy only the first three channels (R, G, B)
    int fromTo[] = {0, 0, 1, 1, 2, 2}; // Map RGBA channels to RGB
    cv::mixChannels(&rgbaImage, 1, &rgbImage, 1, fromTo, 3);

    return rgbImage;
}

inline cv::Mat rgbaToRgbUint(const cv::Mat& rgbaImage) {
    return _rgbaToRgb(rgbaImage, CV_8UC3);
}

inline cv::Mat rgbaToRgbFloat(const cv::Mat& rgbaImage) {
    return _rgbaToRgb(rgbaImage, CV_32FC3);
}

inline cv::Mat _rgbaToBgr(const cv::Mat& rgbaImage, int opencv_image_format) {
    // Create an output Mat with the same size but only 3 channels (BGR)
    cv::Mat bgrImage(rgbaImage.rows, rgbaImage.cols, opencv_image_format);

    // Use mixChannels to copy only the first three channels (R, G, B)
    int fromTo[] = {0, 2, 1, 1, 2, 0}; // Map RGBA channels to BGR
    cv::mixChannels(&rgbaImage, 1, &bgrImage, 1, fromTo, 3);

    return bgrImage;
}

inline cv::Mat rgbaToBgrUint(const cv::Mat& rgbaImage) {
    return _rgbaToBgr(rgbaImage, CV_8UC3);
}

inline cv::Mat rgbaToBgrFloat(const cv::Mat& rgbaImage) {
    return _rgbaToBgr(rgbaImage, CV_32FC3);
}

#endif /* SHADERTASTIC_RGBA_TO_RGB_H */
