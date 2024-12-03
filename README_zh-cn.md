# 介绍

\[ [English](README.md) | 简体中文 \]

本振动框架提供了一组接口，用于管理和控制振动器。该框架旨在为各种设备提供强大的振动功能，通过可自定义的振动模式、幅度和强度来增强用户体验。该框架还支持跨核 API 调用以控制振动器设备。

## 功能

- 支持多种振动模式和波形。
- 可调节振动的幅度和持续时间。
- 自定义振动序列的灵活重复选项。
- 集成预定义的振动效果。
- 控制振动强度的能力。
- 支持跨核API调用控制振动器设备。
- 可以与其他框架和模块集成。

## 构建依赖

- 需开启的配置：
    - 振动器框架侧配置
        - 开启振动器服务（本核）
            ```bash
            VIBRATOR = y
            VIBRATOR_SERVER = y  # 启用振动器服务（vibratord）
            VIBRATOR_SERVER_CPUNAME = "ap"  # （可选）默认主核为'ap'。您可以通过配置选择主核。
            ```
        - 使用振动器服务（本核或其他核）
            ``` bash
            VIBRATOR = y
            ```
        - 日志
            ``` bash
            CONFIG_VIBRATOR_INFO = y
            CONFIG_VIBRATOR_WARN = y
            CONFIG_VIBRATOR_ERROR = y
            ```
    - 振动器测试程序
        ```bash
        VIBRATOR_TEST = y  # （可选）
        ```
    - 驱动侧相关配置
        ```bash
        INPUT_FF = y  # 启用力反馈驱动框架支持
        FF_DUMMY = y  # 启用虚拟 FF 设备驱动以测试振动器功能（当没有实际设备时）
        FF_xxx = y   # 启用相应的硬件设备驱动，例如，`FF_AW86225`。
        ```
- 完成力反馈驱动的初始化和注册。
- 启动振动器服务，使用 `vibrator &`。

## 运行时依赖

有一个与振动硬件连接的物理设备板，并且设备驱动已实现并在力反馈系统中正确注册，或者在模拟环境中启用配置以直接测试。

## 使用方法

参考 `vibrator_test.c` 获取有关如何使用振动器的实际示例。

## 文件结构

振动器框架中的主要文件和目录如下：
```tree
├── Android.bp                # Android 的构建配置
├── CMakeLists.txt            # CMake 构建配置文件
├── Kconfig                   # 配置选项
├── Makefile                  # 用于编译项目的构建脚本
├── vibrator_api.c            # 振动器 API 函数的实现
├── vibrator_api.h            # 定义振动器 API 的头文件
├── vibrator_internal.h       # 振动器实现的内部头文件
├── vibrator_server.c         # 处理振动器请求的服务器实现
└── vibrator_test.c           # 演示如何使用振动器 API 的测试文件
```