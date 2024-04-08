@echo off

set C_FILES=k15_d3d12_renderer.cpp
set OUTPUT_FILE_NAME=k15_d3d12_renderer
set BUILD_CONFIGURATION=%1
call build_cl.bat

exit /b 0