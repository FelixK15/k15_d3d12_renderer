@echo off

set C_FILES=..\tests\test_base_imgui.cpp ..\tests\imgui\imgui.cpp ..\tests\imgui\imgui_demo.cpp ..\tests\imgui\imgui_draw.cpp ..\tests\imgui\imgui_tables.cpp ..\tests\imgui\imgui_widgets.cpp ..\tests\imgui\backends\imgui_impl_dx12.cpp ..\tests\imgui\backends\imgui_impl_win32.cpp
set OUTPUT_FILE_NAME=samples
set BUILD_CONFIGURATION=%1
set OUTPUT_FOLDER=..\win32\build
set FILES_TO_COPY=x64\*.dll ..\tests\spinning_cube\smiley.png ..\tests\sponza\Box.gltf ..\tests\sponza\Box0.bin
call build_cl.bat

exit /b 0