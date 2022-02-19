#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <k4abt.h>

#define VERIFY(result, error)                                                                                          \
    if (result != K4A_RESULT_SUCCEEDED)                                                                                \
    {                                                                                                                  \
        printf("%s \n - (File: %s, Function: %s, Line: %d)\n", error, __FILE__, __FUNCTION__, __LINE__);               \
        exit(1);                                                                                                       \
    }

int main()
{
    // k4a_device_t device = NULL;
    // VERIFY(k4a_device_open(0, &device), "Open K4A Device failed!");

    // // Start camera. Make sure depth camera is enabled.
    // k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    // deviceConfig.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    // deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    // VERIFY(k4a_device_start_cameras(device, &deviceConfig), "Start K4A cameras failed!");

    // k4a_calibration_t sensor_calibration;
    // VERIFY(k4a_device_get_calibration(device,
    //                                   deviceConfig.depth_mode,
    //                                   deviceConfig.color_resolution,
    //                                   &sensor_calibration),
    //        "Get depth camera calibration failed!");

    // Open Recorded file (*.kvm)
    int returnCode = 1;
    k4a_playback_t playback = NULL;
    k4a_calibration_t calibration;
    // k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;
    k4a_result_t result;
    k4a_stream_result_t stream_result;

    // Body tracking
    k4abt_tracker_t tracker = NULL;
    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;

    // User Params
    uint64_t start_timestamp_usec = 1000;
    // std::string input_path = "./data/sample.mkv";
    const char *input_path = "../data/sample.mkv";

    // Open playback (.mkv)
    result = k4a_playback_open(input_path, &playback);
    if (result != K4A_RESULT_SUCCEEDED || playback == NULL)
    {
        printf("Failed to open recording %s\n", input_path);
        return 1;
    }

    // Seek for the closest capture to the start timestamp and get the capture
    result = k4a_playback_seek_timestamp(playback, start_timestamp_usec, K4A_PLAYBACK_SEEK_BEGIN);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to seek start timestamp %ld\n", start_timestamp_usec);
        return 1;
    }
    stream_result = k4a_playback_get_next_capture(playback, &capture);
    if (stream_result == K4A_STREAM_RESULT_EOF)
    {
        printf("Reached to EOF!!\n");
        return 1;
    }
    else if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
    {
        printf("Failed to fetch frame\n");
        return 1;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(playback, &calibration))
    {
        printf("Failed to get calibration\n");
        return 1;
    }

    // Create body tracker
    result = k4abt_tracker_create(&calibration, tracker_config, &tracker);
    if (K4A_RESULT_SUCCEEDED != k4abt_tracker_create(&calibration, tracker_config, &tracker))
    {
        printf("Failed to create body tracker\n");
        return 1;
    }

    // run body tracking
    k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(tracker, capture, K4A_WAIT_INFINITE);
    k4a_capture_release(capture); // Remember to release the sensor capture once you finish using it
    if (queue_capture_result == K4A_WAIT_RESULT_TIMEOUT)
    {
        // It should never hit timeout when K4A_WAIT_INFINITE is set.
        printf("Error! Add capture to tracker process queue timeout!\n");
        return 0;
    }
    else if (queue_capture_result == K4A_WAIT_RESULT_FAILED)
    {
        printf("Error! Add capture to tracker process queue failed!\n");
        return 0;
    }

    k4abt_frame_t body_frame = NULL;
    k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(tracker, &body_frame, K4A_WAIT_INFINITE);
    if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
    {
        // Successfully popped the body tracking result. Start your processing
        size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
        printf("%zu bodies are detected!\n", num_bodies);

        // === Custom Processing ==
        for (size_t i = 0; i < num_bodies; i++)
        {
            k4abt_skeleton_t skeleton;
            k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);
            uint32_t id = k4abt_frame_get_body_id(body_frame, i);

            // Check the contains of skeleton
            float xyz[3] = { 0, 0, 0 };
            float v[3] = { 0, 0, 0 };
            printf("body ID: %d\n", id);
            for (size_t j = 0; j < 33; j++)
            {
                xyz[0] = skeleton.joints[j].position.xyz.x;
                xyz[1] = skeleton.joints[j].position.xyz.y;
                xyz[2] = skeleton.joints[j].position.xyz.z;
                std::cout << "== " << j << " ==" << std::endl;
                std::cout << "xyz:" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ", " << std::endl;
                std::cout << "v  :" << skeleton.joints[j].position.v[0] << ", " << skeleton.joints[j].position.v[1]
                          << ", " << skeleton.joints[j].position.v[2] << std::endl;
            }
        }

        k4abt_frame_release(body_frame); // Remember to release the body frame once you finish using it
    }
    else if (pop_frame_result == K4A_WAIT_RESULT_TIMEOUT)
    {
        //  It should never hit timeout when K4A_WAIT_INFINITE is set.
        printf("Error! Pop body frame result timeout!\n");
        return 1;
    }
    else
    {
        printf("Pop body frame result failed!\n");
        return 1;
    }
    printf("Finished body tracking processing!\n");

    // Clean up
    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    if (playback != NULL)
    {
        k4a_playback_close(playback);
    }
    if (capture != NULL)
    {
        k4a_capture_release(capture);
    }

    return 0;
}