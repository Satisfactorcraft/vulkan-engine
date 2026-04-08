#!/bin/bash
set -e
cd "$(dirname "$0")"

echo "Compiling shaders..."
glslc pbr.vert -o pbr.vert.spv
glslc pbr.frag -o pbr.frag.spv
echo "Done."
