@echo off
"C:\Users\karol\.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-ld.exe" -r build\FocusTimerCore.o build\main.o build\plugin_runtime.o -o build\merged.o
