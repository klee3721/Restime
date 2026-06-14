$ErrorActionPreference = "Stop"

cmake --preset msvc-debug
cmake --build --preset debug
ctest --test-dir build\debug --output-on-failure
