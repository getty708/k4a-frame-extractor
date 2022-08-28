# Body Tracking

- https://docs.microsoft.com/ja-jp/azure/kinect-dk/build-first-body-app
- https://docs.microsoft.com/ja-jp/azure/kinect-dk/get-body-tracking-results

## Usage

```bash
cd build
mkdir outputs
export K4ABT_INPUT_PATH=../data/sample.mkv
export K4ABT_OUTPUT_PATH=./outputs/sample.csv
```


```bash
./k4a_body_tracking 2022-03-04_10:01:22.333 1000 --debug
```