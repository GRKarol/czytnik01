#!/bin/bash
# Build + run the Nano screen simulator: renders every Modern-mode screen
# to tools/nanosim/out/*.ppm (plus *_targets.ppm with the tap targets
# outlined). Needs a host g++ -- on this Windows machine run it in WSL:
#   wsl -d Ubuntu -- bash tools/nanosim/build.sh
set -e
cd "$(dirname "$0")"
FW=../..
mkdir -p out
g++ -std=gnu++17 -O1 -w -DNANO_SIM=1 -Istubs -I$FW/src \
  sim_main.cpp sim_support.cpp sim_screens.cpp sim_plugins.cpp \
  $FW/src/display/DisplayManager.cpp $FW/src/ui/NanoScreens.cpp \
  -o nanosim
./nanosim
