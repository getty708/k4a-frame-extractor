// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "k4a_frame_extraction_helpers.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "turbojpeg.h"

#include <vector>

int write_k4a_color_image(const k4a_image_t color_image, const char *path)
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
    cv::imwrite(path, image);
    return 0;
}

/**
 * @brief Depth画像を16bit PNGで保存する。
 *
 * @param depth_image 深度画像
 * @param file_name 保存するパス (e.g., ./outputs/sample.png)
 */
int write_k4a_depth_image(const k4a_image_t depth_image, const char *path)
{
    // Convert k4a_image_t to cv::Mat
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);
    cv::Mat image(height, width, CV_16UC1);

    uint8_t *depth_image_data = k4a_image_get_buffer(depth_image);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = i * width + j;
            uint8_t val1 = depth_image_data[2 * idx];
            uint8_t val2 = depth_image_data[2 * idx + 1];
            uint16_t val = (uint16_t)val1 + (uint16_t)val2 * 256;
            image.at<uint16_t>(i, j) = val;
        }
    }

    cv::imwrite(path, image);
    return 0;
}

/**
 * @brief 16 bitのdepth画像をBGに8bitずつ記録する.
 *
 * Rは0で埋める。
 *
 * @param depth_image
 * @param file_name
 */
void tranformation_helpers_write_depth_image_3ch(const k4a_image_t depth_image, const char *file_name)
{
    // Convert k4a_image_t to cv::Mat
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);
    cv::Mat image(height, width, CV_8UC3);
    // printf("w=%d, h=%d\n", width, height);

    uint8_t *depth_image_data = k4a_image_get_buffer(depth_image);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = i * width + j;
            image.at<cv::Vec3b>(i, j)[0] = depth_image_data[2 * idx + 0];
            image.at<cv::Vec3b>(i, j)[1] = depth_image_data[2 * idx + 1];
            image.at<cv::Vec3b>(i, j)[2] = 0;
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

/**
 * ====================================================================================
 *   Timestamp
 * ====================================================================================
 */

/**
 * @brief Adjust timestamp delay caused by system.
 *
 * @param rel_ts {unit64_t} device time (= output of k4a_image_get_device_timestamp_usec). Note: this is not an absolute
 * timestamp.
 * @return fixed timestamp [us].
 */
uint64_t fix_timestamp_delay(const uint64_t rel_ts)
{
    // Compute offset
    const uint64_t expected_delay = (uint64_t)(TS_ADJUST_SLOPE * (double)rel_ts);
    uint64_t ts_fixed = rel_ts + expected_delay;

    return ts_fixed;
}

/**
 * @brief  マイクロ秒を timval_delta に変換する.
 *
 * システム上のタイムスタンプの遅延を補正するためには、fix_delay=trueとする。
 *
 * @param ts {uint64_t} マイクロ秒
 * @param tvd {timeval_delta}
 * @param fix_delay {bool} if true, offset will be added to fix sytem delay.
 */
void to_timeval_delta(const uint64_t ts, timeval_delta *tvd, const bool fix_delay = true)
{
    uint64_t ts_ = ts;
    if (fix_delay)
    {
        ts_ = fix_timestamp_delay(ts);
    }

    tvd->tv_sec = (long)ts_ / (1000 * 1000);
    tvd->tv_usec = (long)ts_ % (1000 * 1000);
}

void add_timeval(const struct timeval *tv1, const struct timeval *tv2, struct timeval *tv_out)
{
    long tv_sec = tv1->tv_sec + tv2->tv_sec;
    long tv_usec = tv1->tv_usec + tv2->tv_usec;
    if (tv_usec > (1000 * 1000))
    {
        tv_sec += tv_usec / (1000 * 1000);
        tv_usec = tv_usec % (1000 * 1000);
    }
    tv_out->tv_sec = tv_sec;
    tv_out->tv_usec = tv_usec;
}

/**
 * @brief Parse base time string and convert it into microsecond from the start of the day.
 *
 * @param base_time_str "2021-12-10_10:11:12.000"
 * @return int
 */
int parse_base_timestamp(std::string base_datetime_str, struct timeval *base_tv)
{
    struct tm base_datetime = { 0 };

    // std::string base_time_str = "12:01:02";
    int year = std::stoi(base_datetime_str.substr(0, 4));
    int month = std::stoi(base_datetime_str.substr(5, 2));
    int day = std::stoi(base_datetime_str.substr(8, 2));
    int hour = std::stoi(base_datetime_str.substr(11, 2));
    int minute = std::stoi(base_datetime_str.substr(14, 2));
    int sec = std::stoi(base_datetime_str.substr(17, 2));
    int msec = std::stoi(base_datetime_str.substr(20, 3));
    // printf("%d-%d-%d %d:%d:%d\n", year, month, day, hour, minute, sec);

    base_datetime.tm_year = year - 1900;
    base_datetime.tm_mon = month - 1;
    base_datetime.tm_mday = day;
    base_datetime.tm_hour = hour;
    base_datetime.tm_min = minute;
    base_datetime.tm_sec = sec;
    // printf("BASE DATETIME: %d-%d-%d_%d:%d:%d wday=%d, yday=%d, isdst=%d\n",
    //        base_datetime.tm_year + 1900,
    //        base_datetime.tm_mon + 1,
    //        base_datetime.tm_mday,
    //        base_datetime.tm_hour,
    //        base_datetime.tm_min,
    //        base_datetime.tm_sec,
    //        base_datetime.tm_wday,
    //        base_datetime.tm_yday,
    //        base_datetime.tm_isdst);

    base_tv->tv_sec = mktime(&base_datetime);
    base_tv->tv_usec = msec * 1000;
    return 0;
}

/**
 * ====================================================================================
 *   print
 * ====================================================================================
 */
void print_k4a_calibration_camera_t(k4a_calibration_camera_t *camera)
{
    printf("  - extrinsics:\n");
    printf("    - rotation=[\n");
    for (size_t i = 0; i < 3; i++)
    {
        printf("      [%f, %f, %f]\n",
               camera->extrinsics.rotation[i * 3],
               camera->extrinsics.rotation[i * 3 + 1],
               camera->extrinsics.rotation[i * 3 + 2]);
    }
    printf("    ],\n");
    printf("    - translation=[%f, %f, %f],\n",
           camera->extrinsics.translation[0],
           camera->extrinsics.translation[1],
           camera->extrinsics.translation[2]);

    printf("  - intrinsics={type: %d, parameter_count: %d}\n",
           camera->intrinsics.type,
           camera->intrinsics.parameter_count);
    printf("    - type: %d\n", camera->intrinsics.type);
    printf("    - parameter_count: %d\n", camera->intrinsics.parameter_count);

    printf("  - resolution_width: %d\n", camera->resolution_width);
    printf("  - resolution_height: %d\n", camera->resolution_height);
    printf("  - metric_radius: %f\n", camera->metric_radius);
}

void print_k4a_calibration_t(k4a_calibration_t *calibration)
{
    printf("calibation:\n");
    printf("- depth_camera_calibration:\n");
    print_k4a_calibration_camera_t(&(calibration->depth_camera_calibration));

    printf("- color_camera_calibration:\n");
    print_k4a_calibration_camera_t(&(calibration->color_camera_calibration));

    printf("- depth_mode: %d\n", calibration->depth_mode);
    printf("- color_resolution: %d\n", calibration->color_resolution);
}
