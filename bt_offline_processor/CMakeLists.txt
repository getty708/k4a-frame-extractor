# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
project(offline_processor)


find_package(nlohmann_json 3.7.0 REQUIRED)
# target_link_libraries(configuration PRIVATE nlohmann_json::nlohmann_json)

add_executable(offline_processor
      main.cpp
)

target_link_libraries(offline_processor PRIVATE
    k4a
    k4abt
    k4arecord
    nlohmann_json::nlohmann_json
)