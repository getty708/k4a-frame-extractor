// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <k4a/k4a.h>

void tranformation_helpers_write_color_image(const k4a_image_t color_image, const char *file_name);
void tranformation_helpers_write_depth_image(const k4a_image_t depth_image, const char *file_name);
