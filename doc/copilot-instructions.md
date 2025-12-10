# Copilot / AI Agent Instructions for this repository

Purpose: A concise guide to the project's architecture, common developer workflows,
conventions and hotspots so an AI agent can make safe, focused changes quickly.

- Project layout (high-level)
  - `app/` — application code.
    - `Application/` contains startup, main loop and platform integration.
    - `Game/` holds game objects, scene management and systems (e.g. `render_system`).
    - `Graphic/` implements all DirectX12-related code: resources, managers,
      render passes and pipeline/root-signature builders.
  - `Content/` — prebuilt shader bytecode and runtime assets (`Content/shaders/*.cso`).
  - `shaders/` — HLSL source and shared header types (`*.hlsli`, `*.hlsl`).

- Design & responsibility boundaries (where to look)
  - Managers in `app/Graphic/` control lifetimes and caching: `shader_manager`,
    `texture_manager`, `descriptor_heap_manager`, `swapchain_manager`, `render_pass_manager`.
  - Render-pass system: `app/Graphic/RenderPass/` contains passes (`forward_pass.cpp`,
    `depth_prepass.cpp`, `postprocess_pass.cpp`) and `scene_renderer` orchestrates them.
  - Pipeline & root-signatures: `pipeline_state_builder.cpp` and
    `root_signature_builder.cpp` are the centralized builders for PSOs and roots.
  - GPU resource and upload: `gpu_resource.*`, `upload_context.*`,
    `descriptor_heap_allocator.*` handle allocation, upload and descriptor management.

- Build / run (developer workflow)
  - Preferred toolchain: MSVC with Ninja for development; Visual Studio presets
    for generating a solution when needed.
  - Common PowerShell commands:
    ```powershell
    cmake --preset x64-debug
    cmake --build out/build/x64-debug
    .\out\build\x64-debug\<target>.exe
    ```
  - Visual Studio preset:
    ```powershell
    cmake --preset vs-x64
    cmake --build --preset vs-x64-debug
    ```
  - Output convention: `out/build/<preset>/<Config>/<target>.exe` (x64 only).

- Frequent change hotspots (check before edits)
  - Adding/modifying render passes: `app/Graphic/RenderPass/*` and registration
    order in `render_pass_manager.cpp`.
  - Changing PSO or root signature: `pipeline_state_builder.cpp` &
    `root_signature_builder.cpp`.
  - Descriptor layout or heap behavior: `descriptor_heap_manager.*` and
    `descriptor_heap_allocator.*`.
  - Upload/synchronization: `upload_context.*` and `fence_manager.*`.

- External integrations and assets
  - Third-party helpers: `app/Graphic/ThirdParty/d3dx12.h` and
    `WICTextureLoader12.*` (texture loading).
  - Shader build pipeline is driven by `cmake/shader.cmake` and
    `CMakePresets.json`. When editing HLSL, update compiled bytecode in
    `Content/shaders/` or ensure the shader cmake step is run.

- Verification & common failure modes
  - Changing descriptor tables or root signatures often causes GPU errors.
    Debugging checklist: (1) confirm shader bytecode matches signature, (2)
    validate root signature vs. shader bindings, (3) confirm descriptor table
    allocation and lifetime, (4) verify command ordering and fences.

- PR & commit guidance
  - Make small, focused PRs. Each PR should state the affected files and a
    concise reason (e.g. "add fullscreen postprocess pass" or "adjust descriptor layout").
  - Include build/run steps in the PR description to help reviewers reproduce.

If you want, I can add small code snippets (e.g. minimal render-pass template),
or produce an English/Chinese side-by-side variant. Tell me which example you
need and I will extend this file.