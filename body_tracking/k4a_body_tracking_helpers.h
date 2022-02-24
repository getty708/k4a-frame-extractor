#pragma once

#include <k4a/k4a.h>
#include <k4abt.h>
typedef timeval timeval_delta;

#define TS_ADJUST_SLOPE 0.00042629 // [us/us]

// I/O Utils
int init_output_file_stream();
int close_output_file_stream();
int write_csv_header_row();
int write_row_of_skeleton_joint(const struct timeval *tv, size_t body_index, k4abt_joint_t *joints);

// Timestamp
void to_timeval_delta(const uint64_t ts, timeval_delta *tvd, const bool fix_delay);
void add_timeval(const struct timeval *tv1, const struct timeval *tv2, struct timeval *tv_out);
int parse_base_timestamp(std::string base_datetime_str, struct timeval *base_tv);

uint64_t get_color_frame_timestamp(k4a_capture_t capture, const struct timeval *base_tv, struct timeval *tv);