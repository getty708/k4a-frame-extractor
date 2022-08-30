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


## Data Extraction

```bash
export K4ABT_USER=U0102
export K4ABT_SESSION=S0100
export K4ABT_DATE=2021-10-22_11:15:44.733
export K4ABT_INPUT_PATH=/mnt/ops-ds/open-pack/raw/${K4ABT_USER}/kinect/${K4ABT_SESSION}.mkv 
export K4ABT_OUTPUT_DIR=/mnt/data0/open-pack/interim/${K4ABT_USER}/kinect/3d-kpt
export K4ABT_OUTPUT_PATH=${K4ABT_OUTPUT_DIR}/${K4ABT_SESSION}.csv

./k4a_body_tracking ${K4ABT_DATE} 1000 --debug
```


```bash
mkdir -p ${K4ABT_OUTPUT_DIR}
```