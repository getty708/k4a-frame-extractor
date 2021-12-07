// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "transformation_helpers.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>

#include <vector>

void tranformation_helpers_write_color_image_as_jpeg(const k4a_image_t color_image, const char *file_name)
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