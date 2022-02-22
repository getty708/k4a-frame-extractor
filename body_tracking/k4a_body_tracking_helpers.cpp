#include <string>
#include <iostream>

#include "k4a_body_tracking_helpers.h"

/**
 * ====================================================================================
 *   I/O
 * ====================================================================================
 */
int write_row_of_skeleton_joint(const struct timeval *tv, size_t body_index, k4abt_joint_t *joints)
{
    std::cout << tv->tv_sec << ", " << tv->tv_usec;
    std::cout << ", " << body_index;
    for (size_t i = 0; i < K4ABT_JOINT_COUNT; i++)
    {
        // position
        for (size_t j = 0; j < 3; j++)
        {
            std::cout << ", " << joints[i].position.v[j];
        }
        // orioentation
        for (size_t j = 0; j < 4; j++)
        {
            std::cout << ", " << joints[i].orientation.v[j];
        }
        // confidence_level
        std::cout << ", " << joints[i].confidence_level;
    }

    std::cout << std::endl;
    return 0;
}

/**
 * ====================================================================================
 *   Timestamp
 * ====================================================================================
 */

/**
 * @brief Adjust timestamp delay caused by system.
 *
 * @param rel_ts {unit64_t} device time (= output of k4a_image_get_device_timestamp_usec). Note: this is not an absolute
 * timestamp.
 * @return fixed timestamp [us].
 */
uint64_t fix_timestamp_delay(const uint64_t rel_ts)
{
    // Compute offset
    const uint64_t expected_delay = (uint64_t)(TS_ADJUST_SLOPE * (double)rel_ts);
    uint64_t ts_fixed = rel_ts + expected_delay;

    return ts_fixed;
}

/**
 * @brief  マイクロ秒を timval_delta に変換する.
 *
 * システム上のタイムスタンプの遅延を補正するためには、fix_delay=trueとする。
 *
 * @param ts {uint64_t} マイクロ秒
 * @param tvd {timeval_delta}
 * @param fix_delay {bool} if true, offset will be added to fix sytem delay.
 */
void to_timeval_delta(const uint64_t ts, timeval_delta *tvd, const bool fix_delay = true)
{
    uint64_t ts_ = ts;
    if (fix_delay)
    {
        ts_ = fix_timestamp_delay(ts);
    }

    tvd->tv_sec = (long)ts_ / (1000 * 1000);
    tvd->tv_usec = (long)ts_ % (1000 * 1000);
}

void add_timeval(const struct timeval *tv1, const struct timeval *tv2, struct timeval *tv_out)
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

/**
 * @brief Parse base time string and convert it into microsecond from the start of the day.
 *
 * @param base_time_str "2021-12-10_10:11:12.000"
 * @return int
 */
int parse_base_timestamp(std::string base_datetime_str, struct timeval *base_tv)
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

    base_tv->tv_sec = mktime(&base_datetime);
    base_tv->tv_usec = msec * 1000;
    return 0;
}

// Extract frame at a given timestamp [us] and save file.
uint64_t get_color_frame_timestamp(k4a_capture_t capture = NULL,
                                   const struct timeval *base_tv = { 0 },
                                   struct timeval *tv = { 0 })
{
    // int return_code = 1;
    k4a_image_t color_image = NULL;
    uint64_t color_ts = 0;
    timeval_delta tvd;
    std::string path;

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture @extract_and_write_color_frame()\n");
        k4a_image_release(color_image);
        return 0;
    }
    color_ts = k4a_image_get_device_timestamp_usec(color_image);
    to_timeval_delta(color_ts, &tvd, true);
    add_timeval(base_tv, &tvd, tv);

    k4a_image_release(color_image);
    return color_ts;
}
