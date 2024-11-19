# Introduction

\[ English | [简体中文](README_zh-cn.md) \]

The Vibrator Framework provides a set of interfaces and functionalities for managing and controlling vibrators. This framework is designed to deliver powerful vibration capabilities for various devices, it enhances user experience through customizable vibration patterns, amplitudes, and strengths. This framework also supports cross-core API calls to control the vibrator device.

# Features

- Supports multiple vibration patterns and waveforms.
- Adjustable amplitude and duration of vibrations.
- Flexible repetition options for custom vibration sequences.
- Integrated predefined vibration effects.
- Capability to control vibration intensity.
- Supports cross-core API calls to control the vibrator device.
- Can be integrated with other frameworks and modules.

## Build Dependencies

- Required configurations:
    - Build the vibrator framework
        - open vibrator service(local core)
            ```bash
            VIBRATOR = y
            VIBRATOR_SERVER = y  # Enable the Vibrator service (vibratord)
            VIBRATOR_SERVER_CPUNAME = "ap"  # (Optional) The default main core is the 'ap' core. You can choose the main core through the configuration.
            ```
        - use vibrator service(local or remote core)
            ```bash
            VIBRATOR = y
            ```
        - log
            ```bash
            CONFIG_VIBRATOR_INFO = y
            CONFIG_VIBRATOR_WARN = y
            CONFIG_VIBRATOR_ERROR = y
            ```
    - Build the vibrator test program
        ```bash
        VIBRATOR_TEST = y # (optional)
        ```
    - Driver related configurations
        ```bash
        INPUT_FF = y  # Enable ForceFeedback driver framework support
        FF_DUMMY = y # Enable virtual FF device driver to test Vibrator functionality (when no actual device is present)
        FF_xxx = y  # Enable the corresponding hardware device driver, for example, `FF_AW86225`.
        ```
- Complete the initialization and registration of the force feedback driver.
- Start the vibrator service use `vibrator &`

# Runtime Dependencies

A physical device with a board connected to the vibration hardware, and the device driver has been implemented and correctly registered in the force feedback system, or a simulation environment with configuration enabled for direct testing.

# Usage

Refer to vibrator_test.c for practical examples on how to use the Vibrator

# File Structure

The main files and directories in the Vibrator Framework are as follows:

```tree
├── Android.bp                # Build configuration for Android
├── CMakeLists.txt            # CMake build configuration file
├── Kconfig                   # Configuration options
├── Makefile                  # Build script for compiling the project
├── vibrator_api.c            # Implementation of the vibrator API functions
├── vibrator_api.h            # Header file defining the vibrator API
├── vibrator_internal.h       # Internal header file for the vibrator implementation
├── vibrator_server.c         # Server implementation for handling vibrator requests
└── vibrator_test.c           # Test file demonstrating how to use the Vibrator API
```