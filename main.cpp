// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include <filesystem>
#include <time.h>
namespace fs = std::filesystem;

// FIXME: Rename function that include ``ADLTagger''.
static std::string get_output_path(std::string output_dir, const struct timeval *tv, std::string ext = ".jpg")
{
    uint64_t ts_usec, msec;
    time_t ts_unix;
    struct tm datetime;
    std::string msec_str;

    ts_unix = (time_t)tv->tv_sec;
    localtime_r(&ts_unix, &datetime);

    // directory
    char buf[32];
    strftime(buf, 32, "%Y/%m/%d/%H/%M", &datetime);
    std::string mid_dir = std::string(buf);

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
    std::string fname = std::string(buf) + "_" + msec_str + ext;

    // directory
    std::string dir = output_dir + "/" + mid_dir;
    std::filesystem::create_directories(dir);
    fs::create_directories(dir);

    return dir + "/" + fname;
}

// Extract frame at a given timestamp [us] and save file.
static uint64_t extract_and_write_color_frame(k4a_capture_t capture = NULL,
                                              k4a_transformation_t transformation = NULL,
                                              const struct timeval *base_tv = { 0 },
                                              std::string output_dir = "outputs")
{
    // int return_code = 1;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;
    uint64_t color_ts = 0;
    timeval_delta tvd;
    struct timeval color_tv;
    // std::string dir;
    std::string path;

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture @extract_and_write_color_frame()\n");
        goto ExitC;
    }
    color_ts = k4a_image_get_device_timestamp_usec(color_image);
    to_timeval_delta(color_ts, &tvd);
    add_timeval(base_tv, &tvd, &color_tv);

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
        color_ts = 0;
        return 1;
    }
    if (decompress_color_image(color_image, uncompressed_color_image) != 0)
    {
        printf("Failed to decompress color image @extract_and_write_color_frame()\n");
        color_ts = 0;
        goto ExitC;
    }

    // Compute color point cloud by warping depth image into color camera geometry
    path = get_output_path(output_dir + "/color", &color_tv);
    if (write_k4a_color_image(uncompressed_color_image, path.c_str()) != 0)
    {
        printf("Failed to write color frame\n");
        color_ts = 0;
        goto ExitC;
    }

ExitC:
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    if (uncompressed_color_image != NULL)
    {
        k4a_image_release(uncompressed_color_image);
    }
    return color_ts;
}

// Extract frame at a given timestamp [us] and save file.
static uint64_t extract_and_write_depth_frame(k4a_capture_t capture = NULL,
                                              k4a_transformation_t transformation = NULL,
                                              const struct timeval *base_tv = { 0 },
                                              std::string output_dir = "outputs")
{
    // int return_code = 1;
    k4a_image_t depth_image = NULL;
    uint64_t depth_ts = 0;
    timeval_delta tvd;
    struct timeval depth_tv;
    std::string path;

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture @extract_and_write_depth_frame()\n");
        depth_ts = 0;
        goto ExitD;
    }
    depth_ts = k4a_image_get_device_timestamp_usec(depth_image);
    to_timeval_delta(depth_ts, &tvd);
    add_timeval(base_tv, &tvd, &depth_tv);

    // Compute color point cloud by warping depth image into color camera geometry
    path = get_output_path(output_dir + "/depth", &depth_tv);
    if (write_k4a_depth_image(depth_image, path.c_str()) != 0)
    {
        printf("Failed to transform depth to color @extract_and_write_depth_frame()\n");
        depth_ts = 0;
        goto ExitD;
    }

ExitD:
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    return depth_ts;
}

// Extract frame at a given timestamp [us] and save file.
static uint64_t extract_and_write_depth_frame_color_view(k4a_capture_t capture = NULL,
                                                         k4a_transformation_t transformation_handle = NULL,
                                                         const struct timeval *base_tv = { 0 },
                                                         std::string output_dir = "outputs")
{
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    uint64_t depth_ts = 0;
    timeval_delta tvd;
    struct timeval depth_tv;
    int color_image_width_pixels, color_image_height_pixels;
    k4a_image_t transformed_depth_image = NULL;
    std::string path;

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture @extract_and_write_depth_frame_color_view()\n");
        depth_ts = 0;
        goto ExitDCV;
    }
    depth_ts = k4a_image_get_device_timestamp_usec(depth_image);
    to_timeval_delta(depth_ts, &tvd);
    add_timeval(base_tv, &tvd, &depth_tv);

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture @extract_and_write_depth_frame_color_view()\n");
        depth_ts = 0;
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
        printf("Failed to create transformed depth image @extract_and_write_depth_frame_color_view()\n");
        goto ExitDCV;
    }
    if (K4A_RESULT_SUCCEEDED !=
        k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
    {
        printf("Failed to transform depth to color @extract_and_write_depth_frame_color_view()\n");
        depth_ts = 0;
        goto ExitDCV;
    }

    // Compute color point cloud by warping depth image into color camera geometry
    path = get_output_path(output_dir + "/depth", &depth_tv);
    if (write_k4a_depth_image(transformed_depth_image, path.c_str()) != 0)
    {
        printf("Failed to write depth frame (color view) @extract_and_write_depth_frame_color_view()\n");
        depth_ts = 0;
        goto ExitDCV;
    }

ExitDCV:
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    return depth_ts;
}

// Timestamp in milliseconds. Defaults to 1 sec as the first couple frames don't contain color
static int playback(char *input_path,
                    std::string base_datetime_str = "2020-01-01_00:00:00",
                    std::string output_dir = "./outputs",
                    int start_timestamp = 1000,
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
    uint64_t start_timestamp_usec, end_timestamp_usec;
    uint64_t k4a_timestamp = 0, k4a_timestamp_ = 0;
    struct timeval base_tv;
    long cnt = 0;
    time_t processing_start_time;

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
        end_timestamp_usec = start_timestamp_usec + 5 * 1000000; // 1 sec
    }
    else if (debug == "--debug-long")
    {
        end_timestamp_usec = start_timestamp_usec + 60 * 1000000; // 60 sec
    }
    else
    {
        end_timestamp_usec = recording_length_usec;
    }
    printf("Start Point: %.6lf [sec]\n", (double)start_timestamp_usec / 1000000.0);
    printf("End   Point: %.6lf [sec]\n", (double)end_timestamp_usec / 1000000.0);

    // Seek for the closest capture to the start timestamp
    result = k4a_playback_seek_timestamp(playback, start_timestamp_usec, K4A_PLAYBACK_SEEK_BEGIN);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to seek start timestamp %d\n", start_timestamp);
        goto Exit;
    }

    processing_start_time = time(NULL);
    while (k4a_timestamp < end_timestamp_usec)
    {
        if (capture != NULL)
        {
            k4a_capture_release(capture);
        }
        if (transformation != NULL)
        {
            k4a_transformation_destroy(transformation);
        }

        stream_result = k4a_playback_get_next_capture(playback, &capture);
        if (stream_result == K4A_STREAM_RESULT_EOF)
        {
            printf("Reached to EOF!!\n");
            break;
        }
        else if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
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

        k4a_timestamp_ = extract_and_write_color_frame(capture, transformation, &base_tv, output_dir);
        if (k4a_timestamp_ > k4a_timestamp)
        {
            k4a_timestamp = k4a_timestamp_;
        }

        k4a_timestamp_ = extract_and_write_depth_frame(capture, transformation, &base_tv, output_dir);
        if (k4a_timestamp_ > k4a_timestamp)
        {
            k4a_timestamp = k4a_timestamp_;
        }

        cnt++;
        if (cnt % 100 == 0)
        {
            printf("TIME:  %.3lfs [Complete: %.3lfs / %.3lfs (%6.3lf%%), Elapsed Time: %lds]\n",
                   (double)k4a_timestamp / 1000000.0,
                   (double)(k4a_timestamp - start_timestamp_usec) / 1000000.0,
                   (double)(end_timestamp_usec - start_timestamp_usec) / 1000000,
                   (double)(k4a_timestamp - start_timestamp_usec) /
                       (double)(end_timestamp_usec - start_timestamp_usec) * 100,
                   time(NULL) - processing_start_time);
        }
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
