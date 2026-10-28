# ============================================================
#  XiguChatCommon.cmake — 后端公共部署规则
#
#  xiguchat_deploy(<target>) 在目标构建完成后执行：
#    1. 拷贝本服务目录的 config.ini 到可执行文件目录（三平台通用）
#    2. 拷贝运行时 DLL 到可执行文件目录（仅 Windows）
#
#  平台差异说明：
#    · Windows：依赖的 DLL 必须与可执行文件同目录才能被加载，
#      vcpkg 的 DLL 位于 vcpkg_installed/<triplet>/bin，需显式拷贝。
#    · macOS/Linux：动态库由系统加载器按 rpath 自动定位，无需拷贝。
#
#  运行时 DLL 由 $<TARGET_RUNTIME_DLLS>（CMake 3.21+）在构建时
#  自动收集目标实际依赖的全部 DLL，自动适配 Debug/Release 命名，
#  无需手工维护 DLL 清单。
# ============================================================

function(xiguchat_deploy TARGET)
    # ---- 1. config.ini（三平台都需要：服务按当前工作目录读取配置） ----
    # config.ini 含本机地址与密钥，仓库只有 config.ini.example；
    # 缺失时用示例生成一份，clone 后无需手工准备即可构建运行
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
       AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini.example")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.ini.example"
                       "${CMAKE_CURRENT_SOURCE_DIR}/config.ini" COPYONLY)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
                "$<TARGET_FILE_DIR:${TARGET}>/config.ini"
            COMMENT "Copy config.ini -> $<TARGET_FILE_DIR:${TARGET}>")
    endif()

    # ---- 2. 运行时 DLL（仅 Windows；macOS/Linux 由 rpath 自动定位，见文件头） ----
    if(WIN32)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TARGET}>
                $<TARGET_FILE_DIR:${TARGET}>
            COMMAND_EXPAND_LISTS
            COMMENT "Copy runtime DLLs")
    endif()
endfunction()
