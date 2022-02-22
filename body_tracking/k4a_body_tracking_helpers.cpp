#include <string>
#include <iostream>

#include "k4a_body_tracking_helpers.h"

int write_row_of_skeleton_joint(uint64_t timestamp_us, size_t body_index, k4abt_joint_t *joints)
{
    std::cout << timestamp_us;
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