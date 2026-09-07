#!/bin/bash

echo "This script will attempt to make a symbolic link in your OBS plugins folder (~/.config/obs-studio/plugins/) to the build folder"

DIR=$(dirname $0)
set -e

rm -rf ~/.config/obs-studio/plugins/shadertastic || true

mkdir -p ~/.config/obs-studio/plugins/shadertastic/bin
ln -s "${DIR}/data" ~/.config/obs-studio/plugins/shadertastic/data
ln -s "${DIR}/cmake-build-debug" ~/.config/obs-studio/plugins/shadertastic/bin/64bit

echo "Done !"
