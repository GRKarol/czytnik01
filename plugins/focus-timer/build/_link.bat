@echo off
"C:\Users\karol\.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-ld.exe" -nostdlib -T..\plugin.ld -Map=build\plugin.map build\merged.o -o build\plugin.elf
