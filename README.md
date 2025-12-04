# Game with engine

## Code Style

The code style is flexible and may evolve during development.
All formatting and linting rules are defined in `.clang-format` and `.clang-tidy`; refer to those files for full details.

### Principles

* Maintain consistent style across the entire project.
* Prevent bugs through clear, stable, and predictable coding patterns.
* Prioritize readability over performance in all non-hot paths.
* Naming must be self-explanatory.
  * Name length is not a constraint; longer names are acceptable.
  * However, if a name becomes excessively long, consider whether the function or method is taking too much responsibility and should be refactored.
* Comments should be used only for:
  * algorithms or non-trivial logic
  * workarounds
  * TODO items
  * exceptional cases requiring clarification
  * brief reminders of specific API behavior (not full explanations)
* Write code in a style that both users and maintainers can easily understand.

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
