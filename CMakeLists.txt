cmake_minimum_required (VERSION 3.6)

project(CrownDiligentEngine CXX)

add_subdirectory(DiligentCore)
add_subdirectory(DiligentTools)
add_subdirectory(DiligentFX)

add_executable(CrownDiligentEngine WIN32 CrownDiligentEngine.cpp)
target_compile_options(CrownDiligentEngine PRIVATE -DUNICODE -DENGINE_DLL)

target_link_options(CrownDiligentEngine PRIVATE ${SUBSYSTEM_LINKER_OPTIONS})

target_link_libraries(CrownDiligentEngine
PRIVATE
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineOpenGL-shared
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
)
copy_required_dlls(CrownDiligentEngine)