// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "transformation_helpers.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "turbojpeg.h"

#include <vector>

void tranformation_helpers_write_color_image(const k4a_image_t color_image, const char *file_name)
{
    // Convert k4a_image_t to cv::Mat
    int width = k4a_image_get_width_pixels(color_image);
    int height = k4a_image_get_height_pixels(color_image);
    cv::Mat image(height, width, CV_8UC3);

    uint8_t *color_image_data = k4a_image_get_buffer(color_image);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = i * width + j;
            image.at<cv::Vec3b>(i, j)[0] = color_image_data[4 * idx + 0];
            image.at<cv::Vec3b>(i, j)[1] = color_image_data[4 * idx + 1];
            image.at<cv::Vec3b>(i, j)[2] = color_image_data[4 * idx + 2];
        }
    }
    cv::imwrite(file_name, image);
}

void tranformation_helpers_write_depth_image(const k4a_image_t depth_image, const char *file_name)
{
    // Convert k4a_image_t to cv::Mat
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);
    cv::Mat image(height, width, CV_8UC1);
    // printf("w=%d, h=%d\n", width, height);

    uint8_t *depth_image_data = k4a_image_get_buffer(depth_image);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = i * width + j;
            uint8_t val1 = depth_image_data[2 * idx];
            uint8_t val2 = depth_image_data[2 * idx + 1];
            uint16_t val3 = (uint16_t)val1 + (uint16_t)val2 * 256;
            // BUGFIX: Where does 5000 come from?
            uint8_t val = ((float)val3 / 5000.0) * 256.;
            image.at<uint8_t>(i, j) = val;
        }
    }

    cv::imwrite(file_name, image);
}

/**
 * Convert color frame from mjpeg to bgra by libjpegturb
 */
int decompress_color_image(const k4a_image_t color_image, k4a_image_t uncompressed_color_image)
{
    // printf("FUNC: decompress_color_image\n");
    int color_width, color_height;
    color_width = k4a_image_get_width_pixels(color_image);
    color_height = k4a_image_get_height_pixels(color_image);

    k4a_image_format_t format;
    format = k4a_image_get_format(color_image);
    if (format != K4A_IMAGE_FORMAT_COLOR_MJPG)
    {
        printf("Color format not supported. Please use MJPEG\n");
        return 1;
    }

    tjhandle tjHandle;
    tjHandle = tjInitDecompress();
    if (tjDecompress2(tjHandle,
                      k4a_image_get_buffer(color_image),
                      static_cast<unsigned long>(k4a_image_get_size(color_image)),
                      k4a_image_get_buffer(uncompressed_color_image),
                      color_width,
                      0, // pitch
                      color_height,
                      TJPF_BGRA,
                      TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
    {
        printf("Failed to decompress color frame\n");
        if (tjDestroy(tjHandle))
        {
            printf("Failed to destroy turboJPEG handle\n");
        }
        return 1;
    }
    if (tjDestroy(tjHandle))
    {
        printf("Failed to destroy turboJPEG handle\n");
    }
    return 0;
}
