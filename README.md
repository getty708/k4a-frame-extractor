# Kinect for Azure Frame Extractor
Extract depth/color/IR frames from kinect recording (.mkv).

## Environment

- Ubuntu 18.04 Desktop
- GeForce RTX 2070

## Install

Install kinect SDK:

```bash
sudo apt install k4a-tools
sudo apt install libk4a1.4-dev
```

Install libjpeg-turbo:
https://github.com/libjpeg-turbo/libjpeg-turbo

OpenCV:

```
$ sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
$ sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libdc1394-22-dev

$ git clone https://github.com/opencv/opencv.git
$ git clone https://github.com/opencv/opencv_contrib.git

$ cd opencv
$ mkdir build
$ cd build
$ cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D INSTALL_C_EXAMPLES=OFF \
    -D OPENCV_EXTRA_MODULES_PATH=/home/management/workspace/opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..

$ make -j4
$ sudo make install
$ sudo ldconfig
$ opencv_version
```

[Reference](https://github.com/getty708/Azure-Kinect-Sensor-SDK/issues/1)


Lint Tools:

```bash
sudo apt install clang-format-6.0
```

VS Code Extentions:
- [clang-format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
- [clang-tidy](https://marketplace.visualstudio.com/items?itemName=notskm.clang-tidy)
- CMake
- CMake Tools

## Usage

```
k4a_frame_extaction <filename.mkv> [datetime: YYYY-mm-dd_HH:MM:SS.fff] \
                    [output_dir] [start timestamp (ms)] [--debug|--debug-long]
```

## Output File Format
- color (RGB): JPEG (.jpg), 1920pt x 1080pt
- depth: 16bit PNG (.png) 
- depth2: 16bit (.png), 1920pt x 1080pt (color view)


## Style Guidelines
We follow the coding style of [Azure-Kinect-Sensor-SDK](https://github.com/getty708/Azure-Kinect-Sensor-SDK/blob/develop/CONTRIBUTING.md#style-guidelines).

Style formatting is enforced as part of check in criteria using [.clang-format](https://github.com/getty708/Azure-Kinect-Sensor-SDK/blob/develop/.clang-format).


## Note

Use `bt_offline_processor` instead of `body_tracking`.