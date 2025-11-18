# Game with engine

## Build and Run

Please use MSVC to compile.

The build path is expected in `{ProjectDir}/out/build`.

Executable output is typically located in: `{ProjectDir}/out/build/<preset>/<Config>/<target>.exe`

(Note: x86/32-bit builds are intentionally not supported, focusing only on x64 as the standard for modern Windows development.)

### Generator: `Ninja` (For development)

| Build       | Config (設定 CMAKE_BUILD_TYPE) | Build (指向資料夾)                    |
| ----------- | ------------------------------ | ------------------------------------- |
| x64 Debug   | `cmake --preset x64-debug`     | `cmake --build out/build/x64-debug`   |
| x64 Release | `cmake --preset x64-release`   | `cmake --build out/build/x64-release` |

### Generator: `Visual Studio` (For the solution file)

The config Generate solution, only need execute once.

| Build       | Config                  | Build                                   |
| ----------- | ----------------------- | --------------------------------------- |
| x64 Debug   | `cmake --preset vs-x64` | `cmake --build --preset vs-x64-debug`   |
| x64 Release | `cmake --preset vs-x64` | `cmake --build --preset vs-x64-release` |
