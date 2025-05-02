@echo off

set C_FILES=..\tests\test_base_imgui.cpp ..\tests\imgui\imgui.cpp ..\tests\imgui\imgui_demo.cpp ..\tests\imgui\imgui_draw.cpp ..\tests\imgui\imgui_tables.cpp ..\tests\imgui\imgui_widgets.cpp ..\tests\imgui\backends\imgui_impl_dx12.cpp ..\tests\imgui\backends\imgui_impl_win32.cpp
set OUTPUT_FILE_NAME=samples
set BUILD_CONFIGURATION=%1
set OUTPUT_FOLDER=..\win32\build
set FILES_TO_COPY=x64\*.dll
call build_cl.bat

set C_FILES=..\tests\render_triangle\render_triangle.cpp
set OUTPUT_FILE_NAME=render_triangle
set OUTPUT_FOLDER=..\win32\build\render_triangle
set FILES_TO_COPY=x64\*.dll ..\tests\render_triangle\*.hlsl
::call build_cl.bat

set C_FILES=..\tests\spinning_cube\spinning_cube.cpp
set OUTPUT_FILE_NAME=spinning_cube
set OUTPUT_FOLDER=..\win32\build\spinning_cube
set FILES_TO_COPY=x64\*.dll ..\tests\spinning_cube\*.hlsl ..\tests\spinning_cube\*.png
::call build_cl.bat

set C_FILES=..\tests\compute_texture\compute_texture.cpp
set OUTPUT_FILE_NAME=compute_texture
set OUTPUT_FOLDER=..\win32\build\compute_texture
set FILES_TO_COPY=x64\*.dll ..\tests\compute_texture\*.hlsl
::call build_cl.bat

exit /b 0