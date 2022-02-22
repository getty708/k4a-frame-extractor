#pragma once

#include <k4a/k4a.h>
#include <k4abt.h>

// #define K4ABT_NUM_KEYPOINTS 33
int write_row_of_skeleton_joint(uint64_t timestamp_us, size_t body_index, k4abt_joint_t *joints);
