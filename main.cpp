// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include <filesystem>
namespace fs = std::filesystem;

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
    // printf("mid_dir@get_path(): %s\n", mid_dir.c_str());

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
    // printf("fname: %s\n", fname.c_str());

    // directory
    std::string dir = output_dir + "/" + mid_dir;
    std::filesystem::create_directories(dir);
    fs::create_directories(dir);

    return dir + "/" + fname;
}

static bool write_k4a_color_frame(k4a_transformation_t transformation_handle,
                                  const k4a_image_t color_image, // Uncompressed Image
                                  const struct timeval *color_tv,
                                  std::string output_dir)
{
    std::string file_name_color = get_output_path(output_dir + "/color", color_tv);
    tranformation_helpers_write_color_image(color_image, file_name_color.c_str());

    return true;
}

static bool write_k4a_depth_frame(const k4a_image_t depth_image, const struct timeval *depth_tv, std::string output_dir)
{
    std::string file_name_depth = get_output_path(output_dir, depth_tv);
    // printf("Path[depth]: %s\n", file_name_depth.c_str());
    tranformation_helpers_write_depth_image(depth_image, file_name_depth.c_str());

    return true;
}

// Extract frame at a given timestamp [us] and save file.
static int extract_and_write_color_frame(k4a_capture_t capture = NULL,
                                         k4a_transformation_t transformation = NULL,
                                         const struct timeval *base_tv = { 0 },
                                         std::string output_dir = "outputs")
{
    int return_code = 1;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;
    uint64_t color_ts;
    timeval_delta tvd;
    struct timeval color_tv;

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto ExitC;
    }
    color_ts = k4a_image_get_device_timestamp_usec(color_image);
    to_timeval_delta(color_ts, &tvd);
    add_timeval(base_tv, &tvd, &color_tv);
    // printf("color_tv: sec=%ld, usec=%ld\n", color_tv.tv_sec, color_tv.tv_usec);
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
        goto ExitC;
    }

    // Compute color point cloud by warping depth image into color camera geometry
    if (write_k4a_color_frame(transformation, uncompressed_color_image, &color_tv, output_dir) == false)
    {
        printf("Failed to write color frame\n");
        goto ExitC;
    }
    return_code = 0;

ExitC:
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

// Extract frame at a given timestamp [us] and save file.
static int extract_and_write_depth_frame(k4a_capture_t capture = NULL,
                                         k4a_transformation_t transformation = NULL,
                                         const struct timeval *base_tv = { 0 },
                                         std::string output_dir = "outputs")
{
    int return_code = 1;
    k4a_image_t depth_image = NULL;
    uint64_t depth_ts;
    timeval_delta tvd;
    struct timeval depth_tv;
    std::string dir;

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto ExitD;
    }
    depth_ts = k4a_image_get_device_timestamp_usec(depth_image);
    to_timeval_delta(depth_ts, &tvd);
    add_timeval(base_tv, &tvd, &depth_tv);
    // printf("depth_tv: sec=%ld, usec=%ld\n", depth_tv.tv_sec, depth_tv.tv_usec);

    // Compute color point cloud by warping depth image into color camera geometry
    dir = output_dir + "/depth";
    if (write_k4a_depth_frame(depth_image, &depth_tv, dir) == false)
    {
        printf("Failed to transform depth to color\n");
        goto ExitD;
    }
    return_code = 0;

ExitD:
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    return return_code;
}

// Extract frame at a given timestamp [us] and save file.
static int extract_and_write_depth_frame_color_view(k4a_capture_t capture = NULL,
                                                    k4a_transformation_t transformation_handle = NULL,
                                                    const struct timeval *base_tv = { 0 },
                                                    std::string output_dir = "outputs")
{
    int return_code = 1;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    uint64_t depth_ts;
    timeval_delta tvd;
    struct timeval depth_tv;
    int color_image_width_pixels, color_image_height_pixels;
    k4a_image_t transformed_depth_image = NULL;
    std::string dir;

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto ExitDCV;
    }
    depth_ts = k4a_image_get_device_timestamp_usec(depth_image);
    to_timeval_delta(depth_ts, &tvd);
    add_timeval(base_tv, &tvd, &depth_tv);

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto ExitDCV;
    }

    // transform depth image into color camera geometry
    color_image_width_pixels = k4a_image_get_width_pixels(color_image);
    color_image_height_pixels = k4a_image_get_height_pixels(color_image);
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
                                                 color_image_width_pixels,
                                                 color_image_height_pixels,
                                                 color_image_width_pixels * (int)sizeof(uint16_t),
                                                 &transformed_depth_image))
    {
        printf("Failed to create transformed depth image\n");
        goto ExitDCV;
    }
    if (K4A_RESULT_SUCCEEDED !=
        k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
    {
        printf("Failed to transform depth to color\n");
        goto ExitDCV;
    }

    // Compute color point cloud by warping depth image into color camera geometry
    dir = output_dir + "/depth2";
    if (write_k4a_depth_frame(transformed_depth_image, &depth_tv, dir) == false)
    {
        printf("Failed to write depth frame (color view)\n");
        goto ExitDCV;
    }
    return_code = 0;

ExitDCV:
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    return return_code;
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
    struct timeval base_tv;
    long cnt = 0;

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
            continue;
        }
        if (cnt % (sampling_interval_usec * 100) == 0)
        {
            printf("TIME: %ld / %ld (msec)\n", timestamp_usec / 1000, recording_length_usec / 1000);
        }

        stream_result = k4a_playback_get_next_capture(playback, &capture);
        if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
        {
            printf("Failed to fetch frame\n");
            continue;
        }

        if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(playback, &calibration))
        {
            printf("Failed to get calibration\n");
            continue;
        }

        transformation = k4a_transformation_create(&calibration);

        extract_and_write_color_frame(capture, transformation, &base_tv, output_dir);
        extract_and_write_depth_frame(capture, transformation, &base_tv, output_dir);
        // extract_and_write_depth_frame_color_view(capture, transformation, &base_tv, output_dir);
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
