# dso-console
Based on https://github.com/JakobEngel/dso and https://github.com/Myzhar/qt_camera_calibration.

![photo_2023-11-16_10-45-49](https://github.com/aliakseis/dso-console/assets/11851670/a20c5bf2-05e9-4a10-9ea0-762f8089b326)

# CameraCalibration / DSO Console

A Qt-based camera calibration and visual odometry application integrating **OpenCV**, **GStreamer**, **FFmpeg**, **PCL**, and **Direct Sparse Odometry (DSO)**.

The application provides a complete workflow for connecting to a camera or video source, calibrating its intrinsic and distortion parameters using a chessboard target, previewing the undistorted image, recording the input stream, and running DSO to estimate camera motion and reconstruct a sparse 3D point cloud.

The project is primarily designed as an experimental/research-oriented computer-vision application rather than a minimal calibration utility.

---

## Features

### Camera and video input

The application supports several input backends:

* **GStreamer camera sources**

  * Enumerates available video devices.
  * Detects supported resolutions, pixel formats, and frame rates.
  * Builds a GStreamer pipeline for the selected camera mode.
  * Uses `appsink` to transfer frames into OpenCV.

* **FFmpeg sources**

  * Allows a URL/input source to be decoded through FFmpeg.
  * Useful for files, streams, and sources that are not directly exposed as cameras.

* **Web source**

  * Can obtain frames from a web URL through a browser/Chrome debugging connection.
  * Uses Chrome DevTools Protocol/WebSocket communication.
  * Captured image data is decoded into OpenCV matrices.

The application abstracts these input methods behind camera-thread implementations so that the rest of the processing pipeline works with `cv::Mat` frames.

---

## Camera Calibration

The main purpose of the application is camera calibration.

A standard chessboard calibration workflow is provided:

1. Select a camera or video source.
2. Select the desired resolution and frame rate.
3. Configure the chessboard dimensions.
4. Specify the physical chessboard square size.
5. Start the camera.
6. Present the chessboard to the camera from different orientations and positions.
7. The application detects the chessboard corners.
8. Detected corner sets are accumulated.
9. OpenCV camera calibration is repeatedly performed as new samples arrive.
10. The resulting intrinsic and distortion parameters are displayed in the UI.

The calibration process supports both conventional cameras and fisheye lenses.

### Configurable chessboard parameters

The UI exposes:

* Number of inner corners horizontally.
* Number of inner corners vertically.
* Physical chessboard square size.
* Maximum number of calibration samples.

Detected corners are refined with:

```cpp
cv::cornerSubPix()
```

before being passed to the calibration algorithm.

---

## Camera Models

Two distortion models are supported.

### Standard OpenCV camera model

The standard calibration path uses:

```cpp
cv::calibrateCamera()
```

with:

```cpp
cv::CALIB_RATIONAL_MODEL
```

The application therefore supports an extended distortion vector containing up to eight coefficients:

```text
k1
k2
p1
p2
k3
k4
k5
k6
```

The intrinsic camera matrix is represented as:

```text
[ fx  k01 cx ]
[ k10 fy  cy ]
[ k20 k21  1 ]
```

### Fisheye model

For fisheye cameras the application uses OpenCV's dedicated fisheye calibration implementation:

```cpp
cv::fisheye::calibrate()
```

with four distortion coefficients.

The implementation also uses:

```cpp
cv::fisheye::estimateNewCameraMatrixForUndistortRectify()
cv::fisheye::initUndistortRectifyMap()
```

for image correction.

---

## Image Undistortion

`CameraUndistort` encapsulates the camera model and generates remapping tables used to correct incoming frames.

For conventional cameras:

```cpp
cv::getOptimalNewCameraMatrix()
cv::initUndistortRectifyMap()
```

are used.

For fisheye cameras:

```cpp
cv::fisheye::estimateNewCameraMatrixForUndistortRectify()
cv::fisheye::initUndistortRectifyMap()
```

are used.

Once the maps have been generated, each incoming image can be corrected with:

```cpp
cv::remap()
```

This allows the UI to display both:

* the original camera image;
* the calibrated/undistorted image.

---

## Calibration Parameters

The GUI exposes the major calibration parameters directly.

### Intrinsic parameters

* `fx`
* `fy`
* `cx`
* `cy`
* off-diagonal intrinsic matrix elements
* scale

### Distortion parameters

* `k1`
* `k2`
* `k3`
* `k4`
* `k5`
* `k6`
* `p1`
* `p2`

### Additional parameters

* Image width/height.
* Fisheye mode.
* Undistortion `alpha`.

The calculated reprojection error is also displayed, providing an immediate indication of calibration quality.

---

## Calibration Parameter Persistence

Camera calibration data can be saved and loaded using OpenCV's `FileStorage`.

The stored parameters include:

```text
Width
Height
FishEye
Alpha
CameraMatrix
DistCoeffs
```

This makes calibrated cameras reusable without repeating the chessboard calibration process.

---

## Direct Sparse Odometry

The project integrates **Direct Sparse Odometry (DSO)**.

DSO is used after the camera has been calibrated and provides:

* camera motion estimation;
* sparse scene reconstruction;
* tracked feature/point information;
* a sparse 3D point cloud.

The application creates a DSO `FullSystem` and configures its optimization parameters directly from the GUI.

Several DSO settings are exposed:

```text
Immature point density
Point density
Minimum frames
Maximum frames
Maximum optimization iterations
Minimum optimization iterations
```

The application also configures DSO for the calibrated camera model through its undistortion infrastructure.

---

## Sparse Point Cloud Export

The DSO output is connected to a custom output wrapper.

As DSO generates reconstructed points, the application collects:

```cpp
std::array<float, 3>
```

coordinates.

The accumulated points can then be exported as a PCD file using the Point Cloud Library:

```cpp
pcl::io::savePCDFile()
```

The resulting file contains a `pcl::PointCloud<pcl::PointXYZ>`.

This makes the reconstructed sparse geometry available to external 3D-processing tools.

---

## Video Recording

The application includes a background video recording subsystem.

`VideoSaver` receives camera frames and places them into a bounded queue:

```cpp
FQueue<cv::Mat, 15000000, 500>
```

A dedicated worker thread consumes the frames and encodes them with FFmpeg.

The recording pipeline converts:

```text
BGR24
   ↓
FFmpeg swscale
   ↓
YUV420P
   ↓
H.264
   ↓
MP4
```

The encoder is implemented in `FfmpegEncoder`.

Configurable encoder parameters include:

* frame dimensions;
* frame rate;
* bitrate;
* source pixel format;
* destination pixel format;
* preset;
* CRF.

The default recording configuration uses the `veryfast` preset and CRF 27.

---

## Automatic Video Slicing

Long recordings can be split into multiple MP4 files.

The `Slice duration` setting causes `VideoSaver` to periodically close the current encoder and start another file.

Output filenames are generated from the current date/time:

```text
yyMMddhhmmss.mp4
```

For example:

```text
260810182530.mp4
```

This is useful for continuous camera acquisition where a single enormous recording is undesirable.

---

## GStreamer Integration

The project contains a small OpenCV-oriented GStreamer wrapper:

```text
GstSinkOpenCV
```

Its responsibility is to convert a GStreamer pipeline into a stream of OpenCV frames.

Internally it appends:

```text
videoconvert ! appsink
```

and requests:

```text
video/x-raw,format=BGR
```

from the pipeline.

Frames are copied into a bounded queue and exposed through:

```cpp
cv::Mat getLastFrame();
```

The queue also provides a simple buffer occupancy metric used by the GUI.

### Camera enumeration

The camera manager uses:

```cpp
GstDeviceMonitor
```

to discover video sources.

For every detected device it collects:

* device ID;
* display name;
* GStreamer element;
* supported image sizes;
* supported pixel formats;
* supported frame rates.

This information is used to populate the camera selection UI.

---

## Threaded Frame Acquisition

Camera acquisition is separated from the Qt GUI thread.

The basic architecture is:

```text
Camera / Stream
      │
      ▼
GStreamer / FFmpeg / Web
      │
      ▼
CameraThreadBase
      │
      ▼
cv::Mat
      │
      ├───────────────┐
      ▼               ▼
Calibration       VideoSaver
      │
      ▼
Undistortion
      │
      ▼
DSO
      │
      ▼
Sparse 3D Points
```

This keeps blocking camera/decoder operations away from the main Qt event loop.

---

## Web Video Source

The project contains a browser-based video acquisition path.

`WebThread` communicates with a Chromium/Chrome instance through the remote debugging interface.

The implementation:

1. Connects to Chrome's remote debugging endpoint.
2. Creates/obtains a browser debugging target.
3. Retrieves its WebSocket debugger URL.
4. Establishes a WebSocket connection.
5. Receives encoded image data.
6. Decodes the image with OpenCV.
7. Emits the resulting `cv::Mat`.

The implementation can automatically start Chrome in headless mode when the remote debugging endpoint is unavailable.

---

## FFmpeg Video Source

The FFmpeg input implementation provides an alternative to GStreamer for sources that are better handled by FFmpeg.

The UI exposes:

* source URL;
* input format.

Decoded frames are delivered to the same processing pipeline used by physical cameras.

This allows calibration and DSO processing to be tested against recorded video or network streams.

---

## Qt/OpenCV Visualization

The project contains a reusable `QOpenCVScene` component for displaying OpenCV images inside the Qt interface.

The main window maintains separate views for:

```text
Raw
Undistorted
Chessboard / calibration
```

This makes it possible to compare the original camera image with the corrected image and visually inspect calibration results.

---

## Project Structure

```text
.
├── CMakeLists.txt
├── CameraCalibration.pro
├── LICENSE
├── README.md
├── run.cmd
│
├── cmake/
│   ├── detect_gstreamer.cmake
│   └── version_from_git.cmake
│
├── config/
│   ├── CMakeLists.txt
│   └── config.h.in
│
├── dso/
│   └── DSO dependency
│
├── include/
│   ├── cameraman.h
│   ├── cameraundistort.h
│   ├── CustomLineEdit.h
│   ├── CustomOutputWrapper.h
│   ├── fqueue.h
│   ├── ffmpeg_encode.h
│   ├── mainwindow.h
│   ├── qcameracalibrate.h
│   ├── qchessboardelab.h
│   └── VideoSaver.h
│
├── libs/
│   ├── gst_sink_opencv/
│   │   ├── include/
│   │   └── src/
│   │
│   ├── qt_opencv_scene/
│   │   ├── include/
│   │   └── src/
│   │
│   └── opencv.pri
│
├── src/
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.ui
│   ├── cameraman.cpp
│   ├── cameraundistort.cpp
│   ├── qcameracalibrate.cpp
│   ├── qchessboardelab.cpp
│   ├── ffmpeg_encode.cpp
│   ├── VideoSaver.cpp
│   ├── CustomLineEdit.cpp
│   └── CustomOutputWrapper.cpp
│
├── sound/
│   └── cell-phone-1-nr0.wav
│
└── resources.qrc
```

---

## Main Components

| Component             | Responsibility                                   |
| --------------------- | ------------------------------------------------ |
| `MainWindow`          | Main application UI and processing orchestration |
| `CameraManager`       | Camera discovery and capability enumeration      |
| `CameraThread`        | GStreamer camera acquisition                     |
| `FFmpegThread`        | FFmpeg-based input                               |
| `WebThread`           | Browser/WebSocket video acquisition              |
| `GstSinkOpenCV`       | GStreamer → OpenCV bridge                        |
| `QCameraCalibrate`    | Chessboard-based camera calibration              |
| `QChessboardElab`     | Chessboard detection and corner refinement       |
| `CameraUndistort`     | Camera model and image correction                |
| `VideoSaver`          | Asynchronous video recording                     |
| `FfmpegEncoder`       | Low-level FFmpeg H.264 encoder                   |
| `CustomOutputWrapper` | DSO point-cloud/output integration               |
| `QOpenCVScene`        | OpenCV image display inside Qt                   |
| `FQueue`              | Producer/consumer frame queue                    |

---

## Technology Stack

### C++

The application is written in C++ and configured for:

```text
C++17
```

### Qt

Used for:

* application/UI;
* widgets;
* threading;
* signals/slots;
* settings;
* networking;
* WebSockets;
* multimedia functionality.

The build system is prepared for both Qt 5 and Qt 6.

### OpenCV

Used for:

* camera calibration;
* chessboard detection;
* corner refinement;
* fisheye calibration;
* image undistortion;
* image resizing;
* remapping;
* video frame handling;
* image decoding.

### GStreamer

Used for:

* camera enumeration;
* camera capture;
* pipeline construction;
* real-time frame acquisition.

### FFmpeg

Used for:

* alternative video input;
* H.264 encoding;
* MP4 recording;
* pixel-format conversion.

### DSO

Used for:

* direct sparse visual odometry;
* camera tracking;
* sparse reconstruction.

The project expects DSO as the `dso` source directory/submodule.

### PCL

Used for:

* sparse point-cloud representation;
* PCD export.

### Pangolin

Used by the DSO visualization infrastructure.

---

## Dependencies

A typical build requires:

* Qt 5 or Qt 6
* CMake >= 3.16
* C++17 compiler
* OpenCV 4
* GStreamer
* FFmpeg development libraries

  * `avcodec`
  * `avformat`
  * `avutil`
  * `avdevice`
  * `swscale`
  * `swresample`
* Point Cloud Library (PCL)
* Pangolin
* DSO
* Eigen/Sophus and other dependencies required by DSO

On Windows, the project additionally expects the GStreamer runtime to be available.

---

## DSO Dependency

The repository declares DSO as a Git submodule:

```ini
[submodule "dso"]
    path = dso
    url = https://github.com/aliakseis/dso
```

Initialize it with:

```bash
git submodule update --init --recursive
```

The uploaded source tree contains the `dso/` directory as a submodule location rather than the DSO implementation itself.

---

## Building with CMake

The project uses CMake as its primary modern build system.

A typical configuration is:

```bash
cmake -S . -B build
cmake --build build --config Release
```

The exact configuration depends on where Qt, OpenCV, PCL, FFmpeg, GStreamer, Pangolin, and their development files are installed.

For example, on systems using a custom Qt installation:

```bash
cmake -S . -B build \
    -DCMAKE_PREFIX_PATH=/path/to/Qt
```

Additional include/library paths may be necessary for FFmpeg, GStreamer, OpenCV, PCL, and Pangolin.

---

## Building with Qt Creator / qmake

The project also contains:

```text
CameraCalibration.pro
```

and can therefore be opened with Qt Creator.

The qmake project references the local helper `.pri` files for OpenCV and the included GStreamer/OpenCV components.

CMake is generally preferable for a new build because the current project configuration contains substantially more dependency and deployment logic than the older `.pro` file.

---

## Windows Deployment

The CMake configuration contains Windows-specific installation logic.

It can:

* deploy Qt libraries using `windeployqt`;
* copy the GStreamer runtime;
* install the application executable;
* install the Microsoft runtime components;
* prepare a portable application directory.

The repository also contains:

```text
run.cmd
```

which prepends the bundled GStreamer `bin` directory to `PATH` before starting the application:

```cmd
set "PATH=%~dp0gst\bin\;%PATH%"
"%~dp0\"CameraCalibration.exe
```

A deployed Windows directory can therefore keep the GStreamer runtime alongside the executable.

---

## Typical Workflow

### 1. Start the application

Launch:

```text
CameraCalibration.exe
```

### 2. Select a source

Choose one of:

```text
GStreamer source
FFmpeg source
Web source
```

For a physical camera, select the camera and supported resolution/FPS.

### 3. Configure the chessboard

For example:

```text
Columns:     10
Rows:         7
Square size:  25 mm
Max samples: 15
```

The values must correspond to the actual calibration target.

### 4. Start the camera

Press:

```text
Start Camera
```

The raw camera feed becomes available.

### 5. Calibrate

Press:

```text
Calibrate
```

Move the chessboard through different positions and orientations.

The application detects the corners and updates the camera parameters.

### 6. Inspect the calibration

Compare:

```text
Raw
```

with:

```text
Undistorted
```

Adjust the undistortion `Alpha` value if necessary.

### 7. Save calibration

Save the calculated camera parameters for later use.

### 8. Start DSO

Once the camera has been calibrated and the input is running, enable:

```text
Start DSO
```

DSO will begin tracking the camera and reconstructing sparse scene geometry.

### 9. Export the point cloud

Use:

```text
Save Point Cloud
```

to write the reconstructed points as:

```text
.pcd
```

---

## Calibration Data Flow

The calibration pipeline can be summarized as:

```text
Camera
  │
  ▼
GStreamer / FFmpeg / Web
  │
  ▼
cv::Mat
  │
  ▼
Grayscale conversion
  │
  ▼
Chessboard detection
  │
  ▼
cornerSubPix()
  │
  ▼
Calibration samples
  │
  ▼
cv::calibrateCamera()
       or
cv::fisheye::calibrate()
  │
  ▼
Camera Matrix + Distortion
  │
  ▼
Undistortion maps
  │
  ▼
cv::remap()
```

---

## DSO Data Flow

The visual-odometry path is approximately:

```text
Camera frame
     │
     ▼
Camera calibration
     │
     ▼
Undistorted image
     │
     ▼
DSO FullSystem
     │
     ├── Camera tracking
     ├── Keyframe management
     ├── Sparse optimization
     └── Point reconstruction
             │
             ▼
       CustomOutputWrapper
             │
             ▼
       3D point collection
             │
             ▼
            PCL
             │
             ▼
          .PCD file
```

---

## Recording Data Flow

```text
Camera / Stream
      │
      ▼
    cv::Mat
      │
      ▼
   VideoSaver
      │
      ▼
   FQueue<cv::Mat>
      │
      ▼
Background encoder
      │
      ▼
  FfmpegEncoder
      │
      ├── BGR24
      │
      ├── sws_scale()
      │
      ▼
   YUV420P
      │
      ▼
     H.264
      │
      ▼
     MP4
```

Because encoding happens asynchronously, camera acquisition does not need to perform the expensive H.264 encoding operation directly.

---

## Design Characteristics

Several aspects of the implementation are particularly useful for experimentation.

### Backend-independent processing

The processing code consumes OpenCV matrices rather than depending directly on the source implementation.

Consequently:

```text
GStreamer
FFmpeg
Web
```

can all feed the same calibration/DSO/video-recording pipeline.

### Precomputed undistortion

Undistortion maps are generated when calibration parameters change rather than recalculated for every frame.

This makes runtime image correction substantially cheaper than repeatedly computing camera-model transformations.

### Asynchronous video recording

Recording is decoupled from acquisition using a producer/consumer queue and a background encoder.

### Incremental calibration

Calibration is updated as new chessboard observations arrive, allowing the user to observe the reprojection error and parameter evolution during acquisition.

### DSO integration

Rather than treating DSO as a standalone executable, the project integrates it directly into the Qt application and connects its output to application-owned point-cloud storage.

---

## Limitations / Considerations

This project combines several large native dependencies, so building it from source requires a correctly configured development environment.

In particular:

* GStreamer runtime and development packages must match the application's architecture.
* FFmpeg headers and libraries must be available to CMake.
* OpenCV 4 is required.
* PCL is required for point-cloud export.
* DSO must be available in the `dso` submodule.
* Pangolin is used by the DSO viewer.
* The Web source relies on Chrome/Chromium remote debugging behavior.
* The qmake project is older than the current CMake configuration and should not necessarily be considered feature-equivalent.
* The source contains some platform-specific deployment logic, particularly for Windows.

The application is best regarded as a **research/prototyping tool for camera calibration and monocular visual odometry**.

---

## Possible Applications

The project can serve as a foundation for:

* camera calibration experiments;
* webcam calibration;
* fisheye camera calibration;
* monocular visual odometry;
* sparse 3D reconstruction;
* camera tracking experiments;
* robotics vision pipelines;
* real-time camera diagnostics;
* calibration dataset generation;
* video capture and preprocessing;
* testing DSO with different camera sources.

---

## License

See [`LICENSE`](LICENSE) for the license applicable to this project and its distribution terms.

Third-party components such as OpenCV, GStreamer, FFmpeg, PCL, Pangolin, and DSO remain subject to their respective licenses.

---

## Acknowledgements

This project brings together several established open-source computer-vision and multimedia technologies:

* Qt
* OpenCV
* GStreamer
* FFmpeg
* Point Cloud Library
* Pangolin
* Direct Sparse Odometry

Special mention should be made of the DSO integration, which provides the direct sparse visual-odometry and reconstruction backend.

---

## Summary

**CameraCalibration / DSO Console** is a multifunctional C++/Qt computer-vision application combining camera calibration, lens undistortion, real-time video acquisition, asynchronous recording, direct sparse visual odometry, and sparse point-cloud export.

Its overall architecture is:

```text
                    ┌─────────────────┐
                    │ Camera / Stream │
                    └────────┬────────┘
                             │
             ┌───────────────┼────────────────┐
             │               │                │
             ▼               ▼                ▼
        GStreamer         FFmpeg             Web
             │               │                │
             └───────────────┼────────────────┘
                             ▼
                         OpenCV Mat
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
        Calibration      Recording         DSO
              │              │              │
              ▼              ▼              ▼
       Camera Matrix       FFmpeg       Sparse Points
       + Distortion       H.264/MP4          │
              │                              ▼
              ▼                             PCL
        Undistortion                         │
              │                              ▼
              └──────────────►          PCD file
```

The result is a compact but fairly comprehensive experimental platform for **camera calibration + image correction + video acquisition + monocular sparse reconstruction**.
