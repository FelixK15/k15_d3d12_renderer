@echo off

set C_FILES=..\tests\clear_backbuffer\clear_backbuffer.cpp
set OUTPUT_FILE_NAME=clear_backbuffer
set BUILD_CONFIGURATION=%1
set OUTPUT_FOLDER=..\win32\build\clear_backbuffer
set FILES_TO_COPY=x64\*.dll
::call build_cl.bat

set C_FILES=..\tests\render_triangle\render_triangle.cpp
set OUTPUT_FILE_NAME=render_triangle
set OUTPUT_FOLDER=..\win32\build\render_triangle
set FILES_TO_COPY=x64\*.dll ..\tests\render_triangle\*.hlsl
::call build_cl.bat

set C_FILES=..\tests\spinning_cube\spinning_cube.cpp
set OUTPUT_FILE_NAME=spinning_cube
set OUTPUT_FOLDER=..\win32\build\spinning_cube
set FILES_TO_COPY=x64\*.dll ..\tests\spinning_cube\*.hlsl ..\tests\spinning_cube\*.png
call build_cl.bat

exit /b 0