# CLion 配置指南（成员 1：服务端）

## 问题诊断

当前 `main.cpp` 的 `#include <QCoreApplication>` 报错是因为 CLion 尚未正确检测到 Qt6 路径。

## 解决方案

### 方式一：让 CLion 自动检测 Qt6（推荐）

1. **打开项目**：
   - CLion → `File` → `Open`
   - 选择 `D:\ev-charging-platform\ev-charging-platform\server\CMakeLists.txt`
   - 选择 "Open as Project"

2. **配置 CMake 工具链**：
   - `File` → `Settings` → `Build, Execution, Deployment` → `CMake`
   - 确认 `CMake options` 中没有残留的 `-G` 参数
   - 点击 `-` 删除所有失败的 profile，然后点击 `+` 新建一个 `Debug` profile

3. **指定 Qt6 路径**（如果自动检测失败）：
   - 在 `CMake options` 中添加：
     ```
     -DQt6_DIR=C:/Qt/6.x.x/mingw_64/lib/cmake/Qt6
     ```
   - 将 `6.x.x` 和路径替换为你实际的 Qt6 安装位置
   - 或者使用 MSYS2 安装的 Qt6（如果用 MSYS2）：
     ```
     -DQt6_DIR=C:/msys64/mingw64/lib/cmake/Qt6
     ```

4. **Reload CMake**：
   - `Tools` → `CMake` → `Reload CMake Project`
   - 或点击 CMake 面板上的刷新按钮

### 方式二：使用环境变量

在 CLion 的 CMake 配置中添加环境变量：
- `File` → `Settings` → `Build, Execution, Deployment` → `CMake`
- 在 `Environment` 字段中添加：
  ```
  Qt6_DIR=C:/Qt/6.x.x/mingw_64/lib/cmake/Qt6
  ```

### 方式三：全局安装 Qt6（Ubuntu 目标环境）

如果你打算在 WSL Ubuntu 中开发：
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev cmake build-essential
```
然后在 WSL 环境的 CLion 中打开项目，Qt6 会被自动找到。

## 验证配置成功

配置成功后，CLion 应该：
1. CMake 面板显示绿色勾号，输出 "Build files have been written to..."
2. `main.cpp` 中的 `#include <QCoreApplication>` 不再报错
3. 可以点击右上角的绿色运行按钮编译 `ev-server`

## 当前 CMakeLists.txt 已优化

已添加：
- `CMAKE_EXPORT_COMPILE_COMMANDS ON`：让 CLion 的代码补全更准确
- `Protocol.h` 添加到源文件列表：让 CMake 追踪头文件变更
- Qt6 检测注释：说明目标环境（Ubuntu）不需要手动指定路径

## .gitignore 已更新

已排除：
- `cmake-build-*/`（CLion 默认构建目录）
- `build/`、`CMakeCache.txt`、`CMakeFiles/`（构建产物）
- 所有二进制文件、Qt 自动生成文件、数据库文件

---

**请按以上步骤配置 CLion，成功后告诉我，我将继续 Commit 0 的 0.2-0.7 步骤。**
