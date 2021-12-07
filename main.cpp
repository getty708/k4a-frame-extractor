// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include "turbojpeg.h"
#include <filesystem>
namespace fs = std::filesystem;

// static std::string get_output_filename_from_timestamp(std::string date, int ts)
// {
//     int ts_msec, ts_sec, ts_min, ts_hour;
//     std::string msec_str, sec_str, min_str, hour_str;

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
//     min_str = std::to_string(ts_min);
//     if (ts_min < 10)
//     {
//         min_str = "0" + min_str;
//     }

//     std::string dir = hour_str + "/" + min_str;
//     std::string fname = date + "_" + hour_str + min_str + sec_str;
//     return dir + "/" + fname;
// }

static std::string get_output_path(std::string output_dir, std::string date, uint64_t ts)
{
    uint64_t ts_msec, ts_sec, ts_min, ts_hour;
    std::string msec_str, sec_str, min_str, hour_str;
    printf("data ts: %s, %ld\n", date.c_str(), ts);

    ts = ts / 1000.; // to msec

    // milli second
    ts_msec = ts % 1000;
    ts = ts / 1000;
    msec_str = std::to_string(ts_msec);
    if (ts_msec < 10)
    {
        msec_str = "00" + msec_str;
    }
    else if (ts_msec < 100)
    {
        msec_str = "0" + msec_str;
    }

    // second
    ts_sec = ts % 60;
    ts = ts / 60;
    sec_str = std::to_string(ts_sec);
    if (ts_sec < 10)
    {
        sec_str = "0" + sec_str;
    }

    // minute
    ts_min = ts % 60;
    ts = ts / 60;
    min_str = std::to_string(ts_min);
    if (ts_min < 10)
    {
        min_str = "0" + min_str;
    }

    ts_hour = ts % 60;
    hour_str = std::to_string(ts_hour);
    if (ts_hour < 10)
    {
        hour_str = "0" + hour_str;
    }

    // directory
    std::string dir = output_dir + "/" + hour_str + "/" + min_str;
    // std::filesystem::create_directories(dir);
    // bool result2 =
    fs::create_directories(dir);
    // printf("fs:crate:\n")

    std::string fname = date + "_" + hour_str + min_str + sec_str + ".jpg";

    printf("dir: %s\n", dir.c_str());
    printf("fname: %s\n", fname.c_str());
    return dir + "/" + fname;
}

static bool write_k4a_images(k4a_transformation_t transformation_handle,
                             const k4a_image_t depth_image,
                             uint64_t depth_ts,
                             const k4a_image_t color_image, // Uncompressed Image
                             uint64_t color_ts,
                             std::string output_dir,
                             std::string date)
{
    printf("FUNC: write_k4_images\n");

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

    // Path
    // std::string depth_path_suffix = get_output_filename_from_timestamp(date, depth_ts);
    // std::string color_path_suffix = get_output_filename_from_timestamp(date, color_ts);

    // Write
    std::string file_name_depth = get_output_path(output_dir + "/depth", date, depth_ts);
    printf("Path[depth]: %s\n", file_name_depth.c_str());
    tranformation_helpers_write_depth_image(depth_image, file_name_depth.c_str());

    std::string file_name_depth2 = get_output_path(output_dir + "/depth2", date, depth_ts);
    printf("Path[depth]: %s\n", file_name_depth2.c_str());
    tranformation_helpers_write_depth_image(transformed_depth_image, file_name_depth2.c_str());

    std::string file_name_color = get_output_path(output_dir + "/color", date, color_ts);
    printf("Path[color]: %s\n", file_name_color.c_str());
    tranformation_helpers_write_color_image(color_image, file_name_color.c_str());

    // std::string file_name_color2 = output_dir + "/color2" + depth_path_suffix;
    // tranformation_helpers_write_color_image(transformed_color_image, file_name_color2.c_str());

    k4a_image_release(transformed_depth_image);
    k4a_image_release(transformed_color_image);
    return true;
}

/**
 * Convert color frame from mjpeg to bgra by libjpegturb
 */
static int decompress_color_image(const k4a_image_t color_image, k4a_image_t uncompressed_color_image)
{
    printf("FUNC: decompress_color_image\n");
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

// Extract frame at a given timestamp [us] and save file.
static int extract_and_write_frame(k4a_capture_t capture = NULL,
                                   k4a_transformation_t transformation = NULL,
                                   const std::string date = "000000",
                                   const uint64_t base_timestamp_usec = 0,
                                   std::string output_dir = "outputs")
{
    int return_code = 1;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;
    uint64_t color_timestamp_usec = 0;
    uint64_t depth_timestamp_usec = 0;

    printf("FUNC: extract_and_write_frame\n");

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto ExitA;
    }
    depth_timestamp_usec = k4a_image_get_device_timestamp_usec(depth_image) + base_timestamp_usec;
    printf("ts of depth: %ld, %ld\n", depth_timestamp_usec, base_timestamp_usec);

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto ExitA;
    }
    color_timestamp_usec = k4a_image_get_device_timestamp_usec(color_image) + base_timestamp_usec;
    printf("ts of color: %ld\n", color_timestamp_usec);

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
    if (write_k4a_images(transformation,
                         depth_image,
                         depth_timestamp_usec,
                         uncompressed_color_image,
                         color_timestamp_usec,
                         output_dir,
                         date) == false)
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

/**
 * @brief Parse base time string and convert it into microsecond from the start of the day.
 *
 * @param base_time_str
 * @return int
 */
static uint64_t parse_base_time(std::string base_time_str)
{
    // Configure Filename (timestamp)
    // std::string base_time_str = "12:01:02";
    std::string hour_str = base_time_str.substr(0, 2);
    std::string min_str = base_time_str.substr(3, 2);
    std::string sec_str = base_time_str.substr(6, 2);
    uint64_t base_ts_sec = (uint64_t)(std::stoi(sec_str) + std::stoi(min_str) * 60 + std::stoi(hour_str) * 3600);
    uint64_t base_ts_usec = base_ts_sec * 1000 * 1000;
    printf("BaseTime: %s, %s, %s ==> %ld\n", hour_str.c_str(), min_str.c_str(), sec_str.c_str(), base_ts_usec);

    return base_ts_usec;
}

// Timestamp in milliseconds. Defaults to 1 sec as the first couple frames don't contain color
static int playback(char *input_path,
                    std::string date = "000000",
                    std::string base_time_str = "00:00:00",
                    std::string output_dir = "./outputs",
                    int timestamp = 1000)
{
    int returnCode = 1;
    uint64_t base_timestamp_usec = 0;
    k4a_playback_t playback = NULL;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;

    k4a_result_t result;
    k4a_stream_result_t stream_result;
    uint64_t recording_length_usec;

    printf("FUNC: playback\n");
    base_timestamp_usec = parse_base_time(base_time_str);

    // Open recording
    result = k4a_playback_open(input_path, &playback);
    if (result != K4A_RESULT_SUCCEEDED || playback == NULL)
    {
        printf("Failed to open recording %s\n", input_path);
        goto Exit;
    }

    recording_length_usec = k4a_playback_get_recording_length_usec(playback);
    printf("Recording Length [usec]: %ld\n", recording_length_usec);

    result = k4a_playback_seek_timestamp(playback, timestamp * 1000, K4A_PLAYBACK_SEEK_BEGIN);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to seek timestamp %d\n", timestamp);
        goto Exit;
    }
    printf("Seeking to timestamp: %d/%d (ms)\n",
           timestamp,
           (int)(k4a_playback_get_recording_length_usec(playback) / 1000));

    stream_result = k4a_playback_get_next_capture(playback, &capture);
    if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
    {
        printf("Failed to fetch frame\n");
        goto Exit;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(playback, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }

    transformation = k4a_transformation_create(&calibration);

    extract_and_write_frame(capture, transformation, date, base_timestamp_usec, output_dir);

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
    printf("Usage: transformation_example <filename.mkv> [date: YYYYmmdd] [base_time_str: HH:MM:SS] "
           "[output_dir] [timestamp (ms)]\n");
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
        if (argc == 4)
        {
            returnCode = playback(argv[1], argv[2], argv[3]);
        }
        else if (argc == 5)
        {
            returnCode = playback(argv[1], argv[2], argv[3], argv[4]);
        }
        else if (argc == 6)
        {
            returnCode = playback(argv[1], argv[2], argv[3], argv[4], atoi(argv[5]));
        }
        else
        {
            print_usage();
        }
    }

    return returnCode;
}
