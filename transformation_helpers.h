// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <string>
#include <k4a/k4a.h>
typedef timeval timeval_delta;

int write_k4a_color_image(const k4a_image_t color_image, const char *path);
int write_k4a_depth_image(const k4a_image_t depth_image, const char *path);
void tranformation_helpers_write_depth_image_3ch(const k4a_image_t depth_image, const char *file_name);
int decompress_color_image(const k4a_image_t color_image, k4a_image_t uncompressed_color_image);
void to_timeval_delta(const uint64_t ts, timeval_delta *tvd);
void add_timeval(const struct timeval *tv1, const struct timeval *tv2, struct timeval *tv_out);
int parse_base_timestamp(std::string base_datetime_str, struct timeval *base_tv);
