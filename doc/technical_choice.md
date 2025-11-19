# Technical choice

This note aimed to record the technical choice in this project.

## Project Config

### CMake vs Visual Studio .vcxproj

CMake
- Exploration of new technology
- Compability with extra library
- Use VSCode, CLion, Rider, Visual Studio as developent tools
- Complicated build config

Visual Studio .vcxproj
- Easy management in Visual Studio
- Locked to Visual Studio and Rider
- Hard to implement flag on ci

> This project aimed to try new technology to gain technical experiance, also as a template for future development. To conclude, CMake is a suitable choice for this project.