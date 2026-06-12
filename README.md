# Smart-Traffic-Simulation-
This is a smart traffic simulation using the Raylib library. It features an adaptive intersection that adjusts traffic light timings using density-based algorithms and provides a visual override for ambulances. The UI includes interactive mouse spawning, HUD density bars, and vehicle/road detailed rendering.

## FEATURES

* **Density-Based Adaptive Timing:** Traffic light cycles dynamically scale green light durations between a minimum and maximum window depending on relative vehicle counts in respective lanes.
* **Emergency Vehicle Override:** Detects incoming ambulances and automatically triggers green light override sequences with a "push-ahead" logic that forces leading cars to move out of the way.
* **Realistic Vehicle Dynamics:** Implements smooth linear-interpolation (LERP) deceleration and braking mechanics to maintain realistic vehicle gaps.
* **Intuitive UI/HUD Panel:** Includes real-time density metric bars for North/South vs. East/West lanes alongside an interactive control dashboard.
* **Manual Vehicle Spawning:** Click directly on incoming lanes to dynamically spawn standard cars or ambulances.
* **Polished 2D Visuals:** Detailed road markings, crosswalk stripes, windshield details, and explicit 3-bulb light poles.

## PROJECT STRUCTURE

The project has been refactored into a modular, clean multi-file architecture to decouple data structures, application state logic, and Raylib rendering tasks:

* **`constants.h`** - Holds geometry configurations (screen width/height, road bounds, stop lines) and base timing thresholds.
* **`traffic_light.h` / `.cpp`** - Manages the adaptive timing metrics, intersection phase enums, and state transitions.
* **`vehicle.h` / `.cpp`** - Implements vehicle definitions, spawning safety verification, and smooth gap-based acceleration/braking behaviors.
* **`renderer.h` / `.cpp`** - Encapsulates all drawing pipelines, HUD progress bars, and custom asset rendering logic.
* **`main.cpp`** - Initializes the window context, processes mouse input polling, and loops core simulation update intervals.

## GETTING STARTED

### Prerequisites

You need a C++ toolchain (GCC/Clang or MSVC) and the **Raylib (v4.0 or newer)** development library installed on your system.

* **macOS (via Homebrew):** `brew install raylib`
* **Linux (Ubuntu/Debian):** `sudo apt install libraylib-dev`
* **Windows:** Follow the [Raylib Setup Guide](https://github.com/raysan5/raylib/wiki/Working-on-Windows).

### Compilation

Open your terminal or command prompt inside the project root directory and execute the compilation step matching your setup:

#### Using g++ (Linux / macOS)
```bash
g++ main.cpp traffic_light.cpp vehicle.cpp renderer.cpp -lraylib -O2 -o smart_traffic
