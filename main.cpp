// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include <filesystem>
namespace fs = std::filesystem;

// typedef time_t dtime_t; // 00:00:00.000 からの経過時間 (マイクロ秒, usec)
typedef timeval timeval_delta;

// static std::string get_output_path(std::string output_dir, std::string date, uint64_t ts)
// {
//     uint64_t ts_msec, ts_sec, ts_min, ts_hour;
//     std::string msec_str, sec_str, min_str, hour_str;
//     // printf("data ts: %s, %ld\n", date.c_str(), ts);

//     ts = ts / 1000.; // to msec

//     // milli second
//     ts_msec = ts % 1000;
//     ts = ts / 1000;
//     msec_str = std::to_string(ts_msec);
//     if (ts_msec < 10)
//     {
//         msec_str = "00" + msec_str;
//     }
//     else if (ts_msec < 100)
//     {
//         msec_str = "0" + msec_str;
//     }

//     // second
//     ts_sec = ts % 60;
//     ts = ts / 60;
//     sec_str = std::to_string(ts_sec);
//     if (ts_sec < 10)
//     {
//         sec_str = "0" + sec_str;
//     }

//     // minute
//     ts_min = ts % 60;
//     ts = ts / 60;
//     min_str = std::to_string(ts_min);
//     if (ts_min < 10)
//     {
//         min_str = "0" + min_str;
//     }

//     ts_hour = ts % 60;
//     hour_str = std::to_string(ts_hour);
//     if (ts_hour < 10)
//     {
//         hour_str = "0" + hour_str;
//     }

//     // directory
//     std::string dir = output_dir + "/" + hour_str + "/" + min_str;
//     // std::filesystem::create_directories(dir);
//     // bool result2 =
//     fs::create_directories(dir);
//     // printf("fs:crate:\n")

//     std::string fname = date + "_" + hour_str + min_str + sec_str + "_" + msec_str + ".jpg";

//     // printf("dir: %s\n", dir.c_str());
//     // printf("fname: %s\n", fname.c_str());
//     return dir + "/" + fname;
// }

static std::string get_output_path(std::string output_dir, const struct timeval *tv)
{
    // uint64_t ts_msec, ts_sec, ts_min, ts_hour;
    uint64_t ts_usec, msec;
    time_t ts_unix;
    struct tm datetime;
    std::string msec_str, sec_str, min_str, hour_str;
    // printf("data ts: %s, %ld\n", date.c_str(), ts);

    ts_unix = (time_t)tv->tv_sec;
    localtime_r(&ts_unix, &datetime);

    // directory
    char buf[32];
    strftime(buf, 32, "%Y/%m/%d/%H/%M", &datetime);
    std::string mid_dir = std::string(buf);
    // printf("DATETIME@get_path(): %s\n", buf);
    printf("mid_dir@get_path(): %s\n", mid_dir.c_str());

    // filename
    strftime(buf, 32, "%Y%m%d_%H%M%S", &datetime);
    msec = (uint64_t)(tv->tv_usec / 1000);
    msec_str = std::to_string(msec);
    if (msec_str.length() == 1)
    {
        msec_str = "00" + msec_str;
    }
    else if (msec_str.length() == 2)
    {
        msec_str = "0" + msec_str;
    }
    std::string fname = std::string(buf) + "_" + msec_str + ".jpg";
    printf("fname@get_path(): %s\n", fname.c_str());

    // directory
    std::string dir = output_dir + "/" + mid_dir;
    std::filesystem::create_directories(dir);
    fs::create_directories(dir);

    return dir + "/" + fname;
}

static bool write_k4a_images(k4a_transformation_t transformation_handle,
                             const k4a_image_t depth_image,
                             const struct timeval *depth_tv,
                             const k4a_image_t color_image, // Uncompressed Image
                             const struct timeval *color_tv,
                             std::string output_dir)
{
    // printf("FUNC: write_k4_images\n");

    // transform color image into depth camera geometry
    int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
    int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);
    k4a_image_t transformed_color_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                 depth_image_width_pixels,
                                                 depth_image_height_pixels,
                                                 depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
                                                 &transformed_color_image))
    {
        printf("Failed to create transformed depth image\n");
        k4a_image_release(transformed_color_image);
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_color_image_to_depth_camera(transformation_handle,
                                                                               depth_image,
                                                                               color_image,
                                                                               transformed_color_image))
    {
        printf("Failed to compute transformed depth image\n");
        k4a_image_release(transformed_color_image);
        return false;
    }

    // transform depth image into color camera geometry
    int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
    int color_image_height_pixels = k4a_image_get_height_pixels(color_image);
    k4a_image_t transformed_depth_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
                                                 color_image_width_pixels,
                                                 color_image_height_pixels,
                                                 color_image_width_pixels * (int)sizeof(uint16_t),
                                                 &transformed_depth_image))
    {
        printf("Failed to create transformed depth image\n");
        k4a_image_release(transformed_color_image);
        k4a_image_release(transformed_depth_image);
        return false;
    }
    if (K4A_RESULT_SUCCEEDED !=
        k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
    {
        printf("Failed to compute transformed depth image\n");
        k4a_image_release(transformed_color_image);
        k4a_image_release(transformed_depth_image);
        return false;
    }

    // Write
    std::string file_name_depth = get_output_path(output_dir + "/depth", depth_tv);
    // printf("Path[depth]: %s\n", file_name_depth.c_str());
    tranformation_helpers_write_depth_image(depth_image, file_name_depth.c_str());

    std::string file_name_depth2 = get_output_path(output_dir + "/depth2", depth_tv);
    // printf("Path[depth]: %s\n", file_name_depth2.c_str());
    tranformation_helpers_write_depth_image(transformed_depth_image, file_name_depth2.c_str());

    std::string file_name_color = get_output_path(output_dir + "/color", color_tv);
    // printf("Path[color]: %s\n", file_name_color.c_str());
    tranformation_helpers_write_color_image(color_image, file_name_color.c_str());

    // std::string file_name_color2 = output_dir + "/color2" + depth_path_suffix;
    // tranformation_helpers_write_color_image(transformed_color_image, file_name_color2.c_str());

    k4a_image_release(transformed_depth_image);
    k4a_image_release(transformed_color_image);
    return true;
}

/**
 * @brief  マイクロ秒を timval_delta に変換する.
 *
 * @param ts {uint64_t} マイクロ秒
 * @param tvd {timeval_delta}
 */
static void to_timeval_delta(const uint64_t ts, timeval_delta *tvd)
{
    tvd->tv_sec = (long)ts / (1000 * 1000);
    tvd->tv_usec = (long)ts % (1000 * 1000);
}

static void add_timeval(const struct timeval *tv1, const struct timeval *tv2, struct timeval *tv_out)
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

// Extract frame at a given timestamp [us] and save file.
static int extract_and_write_frame(k4a_capture_t capture = NULL,
                                   k4a_transformation_t transformation = NULL,
                                   const struct timeval *base_tv = { 0 },
                                   std::string output_dir = "outputs")
{
    int return_code = 1;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;
    uint64_t color_ts, depth_ts;
    timeval_delta tvd;
    struct timeval color_tv, depth_tv;

    // printf("FUNC: extract_and_write_frame\n");

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto ExitA;
    }
    depth_ts = k4a_image_get_device_timestamp_usec(depth_image);
    to_timeval_delta(depth_ts, &tvd);
    add_timeval(base_tv, &tvd, &depth_tv);
    printf("depth_tv: sec=%ld, usec=%ld\n", depth_tv.tv_sec, depth_tv.tv_usec);

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto ExitA;
    }
    color_ts = k4a_image_get_device_timestamp_usec(color_image);
    to_timeval_delta(color_ts, &tvd);
    add_timeval(base_tv, &tvd, &color_tv);
    printf("color_tv: sec=%ld, usec=%ld\n", color_tv.tv_sec, color_tv.tv_usec);

    // printf("ts of color: %ld\n", color_timestamp_usec);

    // Decompress Images
    int color_width, color_height;
    color_width = k4a_image_get_width_pixels(color_image);
    color_height = k4a_image_get_height_pixels(color_image);
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                 color_width,
                                                 color_height,
                                                 color_width * 4 * (int)sizeof(uint8_t),
                                                 &uncompressed_color_image))
    {
        printf("Failed to create image buffer\n");
        return 1;
    }
    if (decompress_color_image(color_image, uncompressed_color_image) != 0)
    {
        printf("Failed to decompress color image\n");
        goto ExitA;
    }

    // Compute color point cloud by warping depth image into color camera geometry
    if (write_k4a_images(transformation, depth_image, &depth_tv, uncompressed_color_image, &color_tv, output_dir) ==
        false)
    {
        printf("Failed to transform depth to color\n");
        goto ExitA;
    }
    return_code = 0;

ExitA:
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    if (uncompressed_color_image != NULL)
    {
        k4a_image_release(uncompressed_color_image);
    }
    return return_code;
}

// /**
//  * @brief Parse base time string and convert it into microsecond from the start of the day.
//  *
//  * @param base_time_str
//  * @return int
//  */
// static uint64_t parse_base_time(std::string base_time_str)
// {
//     // Configure Filename (timestamp)
//     // std::string base_time_str = "12:01:02";
//     std::string hour_str = base_time_str.substr(0, 2);
//     std::string min_str = base_time_str.substr(3, 2);
//     std::string sec_str = base_time_str.substr(6, 2);
//     uint64_t base_ts_sec = (uint64_t)(std::stoi(sec_str) + std::stoi(min_str) * 60 + std::stoi(hour_str) * 3600);
//     uint64_t base_ts_usec = base_ts_sec * 1000 * 1000;
//     printf("BaseTime: %s, %s, %s ==> %ld\n", hour_str.c_str(), min_str.c_str(), sec_str.c_str(), base_ts_usec);

//     return base_ts_usec;
// }

/**
 * @brief Parse base time string and convert it into microsecond from the start of the day.
 *
 * @param base_time_str "2021-12-10_10:11:12.000"
 * @return int
 */
static int parse_base_timestamp(std::string base_datetime_str, struct timeval *base_tv)
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
    printf("BASE DATETIME: %d-%d-%d_%d:%d:%d wday=%d, yday=%d, isdst=%d\n",
           base_datetime.tm_year + 1900,
           base_datetime.tm_mon + 1,
           base_datetime.tm_mday,
           base_datetime.tm_hour,
           base_datetime.tm_min,
           base_datetime.tm_sec,
           base_datetime.tm_wday,
           base_datetime.tm_yday,
           base_datetime.tm_isdst);

    base_tv->tv_sec = mktime(&base_datetime);
    base_tv->tv_usec = msec * 1000;
    return 0;
}

// Timestamp in milliseconds. Defaults to 1 sec as the first couple frames don't contain color
static int playback(char *input_path,
                    std::string base_datetime_str = "2020-01-01_00:00:00",
                    std::string output_dir = "./outputs",
                    int start_timestamp = 10000,
                    std::string debug = "")
{
    int returnCode = 1;
    k4a_playback_t playback = NULL;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;

    k4a_result_t result;
    k4a_stream_result_t stream_result;
    uint64_t recording_length_usec;
    uint64_t sampling_interval_usec = 50000; // 100[ms] = 10Hz
    uint64_t start_timestamp_usec;
    // struct tm base_date = { 0 };
    // dtime_t base_dtime = 0;
    struct timeval base_tv;

    printf("FUNC: playback\n");
    parse_base_timestamp(base_datetime_str, &base_tv);
    printf("BASE DATETIME: sec=%ld,  usec=%ld\n", base_tv.tv_sec, base_tv.tv_usec);

    // Open recording
    result = k4a_playback_open(input_path, &playback);
    if (result != K4A_RESULT_SUCCEEDED || playback == NULL)
    {
        printf("Failed to open recording %s\n", input_path);
        goto Exit;
    }

    recording_length_usec = k4a_playback_get_recording_length_usec(playback);
    printf("Recording Length [usec]: %ld\n", recording_length_usec);

    start_timestamp_usec = (uint64_t)start_timestamp * 1000;
    if (debug == "--debug")
    {
        recording_length_usec = start_timestamp_usec + sampling_interval_usec * 20;
    }
    for (uint64_t timestamp_usec = start_timestamp_usec; timestamp_usec < recording_length_usec;
         timestamp_usec = timestamp_usec + sampling_interval_usec)
    {
        if (capture != NULL)
        {
            k4a_capture_release(capture);
        }
        if (transformation != NULL)
        {
            k4a_transformation_destroy(transformation);
        }

        result = k4a_playback_seek_timestamp(playback, timestamp_usec, K4A_PLAYBACK_SEEK_BEGIN);
        if (result != K4A_RESULT_SUCCEEDED)
        {
            printf("Failed to seek timestamp %ld\n", timestamp_usec);
            // goto Exit;
            continue;
        }
        printf("TIME: %ld / %ld (msec)\n", timestamp_usec / 1000, recording_length_usec / 1000);

        stream_result = k4a_playback_get_next_capture(playback, &capture);
        if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
        {
            printf("Failed to fetch frame\n");
            continue;
        }

        if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(playback, &calibration))
        {
            printf("Failed to get calibration\n");
            // goto Exit;
            continue;
        }

        transformation = k4a_transformation_create(&calibration);

        // dtime_t current_timestamp = base_dtime + timestamp_usec;
        extract_and_write_frame(capture, transformation, &base_tv, output_dir);
    }

    returnCode = 0;

Exit:
    if (playback != NULL)
    {
        k4a_playback_close(playback);
    }
    if (capture != NULL)
    {
        k4a_capture_release(capture);
    }
    if (transformation != NULL)
    {
        k4a_transformation_destroy(transformation);
    }
    return returnCode;
}

static void print_usage()
{
    printf("Usage: transformation_example <filename.mkv> [datetime: YYYY-mm-dd_HH:MM:SS] "
           "[output_dir] [timestamp (ms)] [--debug]\n");
}

int main(int argc, char **argv)
{
    int returnCode = 0;

    if (argc < 2)
    {
        print_usage();
    }
    else
    {
        if (argc == 3)
        {
            returnCode = playback(argv[1], argv[2]);
        }
        if (argc == 4)
        {
            returnCode = playback(argv[1], argv[2], argv[3]);
        }
        else if (argc == 5)
        {
            returnCode = playback(argv[1], argv[2], argv[3], atoi(argv[4]));
        }
        else if (argc == 6)
        {
            returnCode = playback(argv[1], argv[2], argv[3], atoi(argv[4]), argv[5]);
        }
        else
        {
            print_usage();
        }
    }

    return returnCode;
}
