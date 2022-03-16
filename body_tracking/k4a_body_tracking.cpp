#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <iomanip>

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <k4abt.h>

#include "k4a_body_tracking_helpers.h"

// Global Variable
k4a_playback_t playback = NULL;
k4abt_tracker_t tracker = NULL;

int init_body_tracker()
{
    k4a_result_t result;
    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result;
    k4a_calibration_t calibration;

    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
    k4abt_frame_t body_frame = NULL;

    // get a first frame
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

    // FIXME: Is calibaration really needed for every frame?
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

    printf("Succeeded in initializing the body tracker.\n");
    return 0;
}

int finalize_body_tracker()
{
    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    printf("Succeeded in destroying the body tracker.\n");
    return 0;
}

uint64_t process_single_frame(const struct timeval *base_tv = { 0 })
{
    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result;
    k4a_calibration_t calibration;
    k4a_wait_result_t queue_capture_result;
    k4a_wait_result_t pop_frame_result;
    k4a_result_t result;

    k4abt_frame_t body_frame = NULL;

    uint64_t color_ts;
    struct timeval tv;

    // get a next frame
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

    // get timestamp of corresponding freame (color_ts, tv)
    color_ts = get_color_frame_timestamp(capture, base_tv, &tv);

    // run body tracking
    queue_capture_result = k4abt_tracker_enqueue_capture(tracker, capture, K4A_WAIT_INFINITE);
    k4a_capture_release(capture);
    if (queue_capture_result == K4A_WAIT_RESULT_TIMEOUT)
    {
        // It should never hit timeout when K4A_WAIT_INFINITE is set.
        printf("Error! Add capture to tracker process queue timeout!\n");
        return color_ts;
    }
    else if (queue_capture_result == K4A_WAIT_RESULT_FAILED)
    {
        printf("Error! Add capture to tracker process queue failed!\n");
        return color_ts;
    }

    // get and write skeleton info in CSV format
    pop_frame_result = k4abt_tracker_pop_result(tracker, &body_frame, K4A_WAIT_INFINITE);
    if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
    {
        // Successfully popped the body tracking result. Start your processing
        size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
        // if (num_bodies != 1)
        // {
        //     std::cout << "[WARNING] " << num_bodies << " bodyies are detected." << std::endl;
        // }

        // === Custom Processing ==
        for (size_t body_index = 0; body_index < num_bodies; body_index++)
        {
            k4abt_skeleton_t skeleton;
            k4abt_frame_get_body_skeleton(body_frame, body_index, &skeleton);
            uint32_t id = k4abt_frame_get_body_id(body_frame, body_index);
            write_row_of_skeleton_joint(&tv, body_index, skeleton.joints);
        }
        k4abt_frame_release(body_frame); // Remember to release the body frame once you finish using it
    }
    else if (pop_frame_result == K4A_WAIT_RESULT_TIMEOUT)
    {
        //  It should never hit timeout when K4A_WAIT_INFINITE is set.
        printf("Error! Pop body frame result timeout!\n");
        return color_ts;
    }
    else
    {
        printf("Pop body frame result failed!\n");
        return color_ts;
    }

    return color_ts;
}

int playback_handler(std::string base_datetime_str = "2020-01-01_00:00:00.000",
                     int start_timestamp = 1000,
                     std::string debug = "")
{
    const char *input_path = getenv("K4ABT_INPUT_PATH");
    std::cout << "Input       : " << input_path << std::endl;

    int returnCode = 1;
    k4a_result_t result;
    uint64_t start_timestamp_usec, end_timestamp_usec, recording_length_usec;
    uint64_t k4a_timestamp = 0;
    struct timeval base_tv;

    long cnt = 0;
    time_t processing_start_time;

    // init output stream
    if (init_output_file_stream() != 0)
    {
        printf("cannot create new file stream.\n");
        return 1;
    }
    write_csv_header_row();

    printf("FUNC        : playback\n");
    parse_base_timestamp(base_datetime_str, &base_tv);
    printf("BASE TIME   : sec=%ld,  usec=%ld\n", base_tv.tv_sec, base_tv.tv_usec);

    // open playback (.mkv)
    result = k4a_playback_open(input_path, &playback);
    if (result != K4A_RESULT_SUCCEEDED || playback == NULL)
    {
        printf("Failed to open recording %s\n", input_path);
        return 1;
    }

    recording_length_usec = k4a_playback_get_recording_length_usec(playback);
    printf("Rec Length [usec]: %ld\n", recording_length_usec);
    start_timestamp_usec = (uint64_t)start_timestamp * 1000;

    // hundle debug option
    if (debug == "--debug")
    {
        end_timestamp_usec = start_timestamp_usec + 2 * 1000000; // 2 sec
    }
    else if (debug == "--debug-long")
    {
        end_timestamp_usec = start_timestamp_usec + 60 * 1000000; // 60 sec
    }
    else
    {
        end_timestamp_usec = recording_length_usec;
    }
    printf("Start Point : %.6lf [sec]\n", (double)start_timestamp_usec / 1000000.0);
    printf("End   Point : %.6lf [sec]\n", (double)end_timestamp_usec / 1000000.0);

    // Seek for the closest capture to the start timestamp and get the capture
    result = k4a_playback_seek_timestamp(playback, start_timestamp_usec, K4A_PLAYBACK_SEEK_BEGIN);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to seek start timestamp %ld\n", start_timestamp_usec);
        return 1;
    }

    init_body_tracker();

    processing_start_time = time(NULL);
    while (k4a_timestamp < end_timestamp_usec)
    {

        k4a_timestamp = process_single_frame(&base_tv);
        if (k4a_timestamp < 0)
        {
            printf("error occured at process_single_frame() with code %ld.\n", k4a_timestamp);
            return 1;
        }

        if (cnt % 1000 == 0)
        {
            const double progress = (double)(k4a_timestamp - start_timestamp_usec) /
                                    (double)(end_timestamp_usec - start_timestamp_usec) * 100.0;
            const double elapsed_time = ((double)time(NULL) - (double)processing_start_time) / 60.0;

            std::cout << "[TIME: " << std::setprecision(3) << std::fixed << elapsed_time << " min] ";
            std::cout << cnt << " frames finised!";
            std::cout << "(" << progress << "%, " << k4a_timestamp << " / " << end_timestamp_usec << "[us])"
                      << std::endl;
        }
        cnt++;
    }

    finalize_body_tracker();
    close_output_file_stream();
    return 0;
}

static void print_usage()
{
    printf("Usage: k4a_body_tracking [datetime: YYYY-mm-dd_HH:MM:SS] "
           "[timestamp (ms)] [--debug|--debug-long]\n");
}

int main(int argc, char **argv)
{
    int returnCode = 0;

    if (argc == 1)
    {
        returnCode = playback_handler();
    }
    else if (argc == 2)
    {
        returnCode = playback_handler(argv[1]);
    }
    else if (argc == 3)
    {
        returnCode = playback_handler(argv[1], atoi(argv[2]));
    }
    else if (argc == 4)
    {
        returnCode = playback_handler(argv[1], atoi(argv[2]), argv[3]);
    }
    else
    {
        print_usage();
    }

    // Clean up
    if (playback != NULL)
    {
        k4a_playback_close(playback);
    }

    return returnCode;
}