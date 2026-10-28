# 同名覆盖 vcpkg 内置的 x64-linux triplet，CMake 预设里的 triplet 名无需改动。
# 唯一差异：只编 Release 变体（项目以 Release 构建，用不到 Debug 依赖）
include("$ENV{VCPKG_ROOT}/triplets/x64-linux.cmake")
set(VCPKG_BUILD_TYPE release)
