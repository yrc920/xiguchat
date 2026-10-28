# vcpkg overlay triplet：与内置 x64-linux 同名，CMake 预设里的 triplet 名照常使用。
# 与内置的差异只有一处：只编 Release 变体（项目以 Release 构建，用不到 Debug 依赖）
include("$ENV{VCPKG_ROOT}/triplets/x64-linux.cmake")
set(VCPKG_BUILD_TYPE release)
