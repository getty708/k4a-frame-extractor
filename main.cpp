// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include "turbojpeg.h"

static bool write_k4a_images(k4a_transformation_t transformation_handle,
                             const k4a_image_t depth_image,
                             uint64_t depth_ts,
                             const k4a_image_t color_image, // Uncompressed Image
                             uint64_t color_ts,
                             std::string output_dir)
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
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_color_image_to_depth_camera(transformation_handle,
                                                                               depth_image,
                                                                               color_image,
                                                                               transformed_color_image))
    {
        printf("Failed to compute transformed depth image\n");
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
        return false;
    }

    // Width
    if (K4A_RESULT_SUCCEEDED !=
        k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
    {
        printf("Failed to compute transformed depth image\n");
        return false;
    }

    std::string file_name_depth = output_dir + "/depth" + "/20211207_120000_000.jpeg";
    tranformation_helpers_write_depth_image(depth_image, file_name_depth.c_str());

    std::string file_name_depth2 = output_dir + "/depth2" + "/20211207_120000_000.jpeg";
    tranformation_helpers_write_depth_image(transformed_depth_image, file_name_depth2.c_str());

    std::string file_name_color = output_dir + "/color" + "/20211207_120000_000.jpeg";
    tranformation_helpers_write_color_image(color_image, file_name_color.c_str());

    std::string file_name_color2 = output_dir + "/color2" + "/20211207_120000_000.jpeg";
    tranformation_helpers_write_color_image(transformed_color_image, file_name_color2.c_str());

    k4a_image_release(transformed_depth_image);
    // k4a_image_release(point_cloud_image);

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
                                   int timestamp_usec = 1000000,
                                   std::string output_dir = "outputs")
{
    int return_code = 1;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;
    uint64_t color_ts = 0;
    uint64_t depth_ts = 0;

    printf("FUNC: extract_and_write_frame\n");

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto ExitA;
    }
    depth_ts = k4a_image_get_device_timestamp_usec(depth_image);
    printf("ts of depth: %ld\n", depth_ts);

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto ExitA;
    }
    color_ts = k4a_image_get_device_timestamp_usec(color_image);
    printf("ts of color: %ld\n", color_ts);

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
    if (write_k4a_images(transformation, depth_image, depth_ts, uncompressed_color_image, color_ts, output_dir) ==
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

// Timestamp in milliseconds. Defaults to 1 sec as the first couple frames don't contain color
static int playback(char *input_path, int timestamp = 1000, std::string output_dir = "outputs")
{
    int returnCode = 1;
    k4a_playback_t playback = NULL;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;

    k4a_result_t result;
    k4a_stream_result_t stream_result;
    uint64_t recording_length_usec;

    printf("FUNC: playback\n");

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

    extract_and_write_frame(capture, transformation, timestamp * 1000, output_dir);

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
    printf("Usage: transformation_example playback <filename.mkv> [timestamp (ms)] [output_file]\n");
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
        std::string mode = std::string(argv[1]);
        if (mode == "playback")
        {
            if (argc == 3)
            {
                returnCode = playback(argv[2]);
            }
            else if (argc == 4)
            {
                returnCode = playback(argv[2], atoi(argv[3]));
            }
            else if (argc == 5)
            {
                returnCode = playback(argv[2], atoi(argv[3]), argv[4]);
            }
            else
            {
                print_usage();
            }
        }
        else
        {
            print_usage();
        }
    }

    return returnCode;
}
