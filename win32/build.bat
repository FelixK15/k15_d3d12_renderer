@echo off

::set C_FILES=..\tests\clear_backbuffer\clear_backbuffer.cpp
::set OUTPUT_FILE_NAME=clear_backbuffer
set C_FILES=..\tests\render_triangle\render_triangle.cpp
set OUTPUT_FILE_NAME=render_triangle
set BUILD_CONFIGURATION=%1
set OUTPUT_FOLDER=..\win32\build
set FILES_TO_COPY=x64\*.dll ..\tests\render_triangle\*.hlsl
call build_cl.bat

exit /b 0