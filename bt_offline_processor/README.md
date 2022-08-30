# Azure Kinect Body Tracking OfflineProcessor Sample

https://github.com/microsoft/Azure-Kinect-Samples/tree/master/body-tracking-samples/offline_processor


## Setup

Insall nlohmann/json:

```bash
sudo apt-get install nlohmann-json3-de
```

Build:

```bash
make -p build && cd build/
cmake ..
make
cd ../
```

## Usage 

```bash
./build/offline_processor <input.mkv> <output.json>
```

