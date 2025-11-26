# QmlPlayer

QmlPlayer is a Qt/QML video player that uses FFmpeg for decoding and a custom OpenGL renderer for displaying video frames via `QQuickFramebufferObject`. The application is designed to be a small, focused example of integrating FFmpeg with Qt Quick and OpenGL on modern Qt 6.

This repository currently targets **Qt 6** and **FFmpeg** on Windows, using **CMake + Ninja** as the primary build setup.

---

## Features

- OpenGL-based video rendering via `QQuickFramebufferObject` and a dedicated `GLVideoRenderer` helper.
- FFmpeg-based video decoding (`VideoDecoder`) with:
  - Playback state management: `Playing`, `Paused`, `Stopped`.
  - Duration and current position reporting in milliseconds.
  - Seek support from QML (dragging the progress slider updates playback position).
- QML-based UI:
  - `VideoRenderer` item registered as a QML type (`VideoPlayer 1.0`) and used as the main video surface.
  - Bottom control panel with **Open**, **Play/Pause**, **Stop**, a **progress slider**, and **time display**.

The feature set is intentionally minimal but provides a solid base to extend into a more full‑featured player (subtitles, audio controls, playlists, etc.).

---

## Build Instructions (Windows)

The project is currently developed and tested on Windows with:

- **Qt:** 6.9.2 (MinGW 64-bit)
- **Compiler:** MinGW-w64 (as provided by winlibs)
- **FFmpeg:** prebuilt DLLs and import libraries (e.g. `libavformat.dll.a`, `libavcodec.dll.a`, `libavutil.dll.a`, `libswscale.dll.a`)
- **CMake** and **Ninja**

### Prerequisites

- Install Qt 6 (including Qt Quick, Qt Quick Controls 2, and OpenGL modules).
- Install FFmpeg and ensure the development headers and libraries are available.
- Ensure CMake and Ninja are on your `PATH`.

Paths to Qt and FFmpeg are currently configured directly in `CMakeLists.txt`. Adjust these to match your local installation if needed.

### Configure and build

From the project root:

```bash
mkdir build
cd build
cmake .. -G "Ninja"
cmake --build .
```

This produces `QMLPlayerFFmpeg.exe` in the `build` directory.

### Run

From the `build` directory:

```bash
./QMLPlayerFFmpeg.exe
```

Then:

1. Click **Open** and choose a video file.
2. Use **Play/Pause/Stop** buttons to control playback.
3. Drag the **progress slider** to seek; release the mouse to jump to the chosen position.

On Windows, make sure FFmpeg DLLs (`avformat`, `avcodec`, `avutil`, `swscale`) are either in the same directory as the executable or in a directory on the system `PATH`, so the application can load them at runtime.

---

## Project Layout

- `src/main.cpp` – Qt application entry point, QML engine setup, QML type registration.
- `src/core/VideoDecoder.{h,cpp}` – FFmpeg-based decoder with playback state, duration/position, and seek.
- `src/core/VideoRenderer.{h,cpp}` – QQuickFramebufferObject-based video item and glue to the decoder.
- `src/resources/qml/main.qml` – Main QML UI, including `VideoRenderer` and control panel.

---

## License

This project does not yet declare an explicit license. If you plan to redistribute or use it in another project, please add an appropriate open‑source license (for example MIT or BSD) and review compatibility with your FFmpeg and Qt distributions.