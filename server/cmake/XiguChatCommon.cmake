# ============================================================
#  XiguChatCommon.cmake — 后端公共部署规则
#
#  xiguchat_deploy(<target>) 在目标构建完成后执行：
#    1. 拷贝本服务目录的 config.ini 到可执行文件目录（三平台通用）
#    2. 拷贝运行时 DLL 到可执行文件目录（仅 Windows）
#
#  平台差异说明：
#    · Windows：将依赖的 DLL 放在可执行文件同目录，便于直接启动，
#      vcpkg 的 Release DLL 位于 <triplet>/bin，Debug DLL 位于 debug/bin。
#    · macOS/Linux：默认 triplet 的多数依赖静态链接；动态依赖通过
#      构建期 rpath 定位。跨机器部署时仍需携带动态库并检查加载路径。
#
#  运行时 DLL 由 $<TARGET_RUNTIME_DLLS>（CMake 3.21+）在构建时
#  收集 CMake 目标依赖关系中可识别的 DLL，自动适配 Debug/Release；
#  vcpkg 的 applocal 还会在链接后解析并拷贝可执行文件的 DLL 依赖。
#  这些规则用于本机构建部署，不会生成独立的跨机器安装包。
# ============================================================

function(xiguchat_deploy TARGET)
    # ---- 1. config.ini（三平台都需要：服务按当前工作目录读取配置） ----
    # config.ini 含本机地址与密钥，仓库只有 config.ini.example；
    # 缺失时用示例生成一份以完成构建，运行前仍需按实际环境填写服务地址与密钥
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
       AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini.example")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.ini.example"
                       "${CMAKE_CURRENT_SOURCE_DIR}/config.ini" COPYONLY)
    endif()
    # POST_BUILD 只在目标实际构建后执行，单独编辑 ini 不一定触发复制；
    # 若只更新运行配置，可以直接修改输出目录中的 config.ini。
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
                "$<TARGET_FILE_DIR:${TARGET}>/config.ini"
            VERBATIM
            COMMENT "Copy config.ini -> $<TARGET_FILE_DIR:${TARGET}>")
    endif()

    # ---- 2. 运行时 DLL（仅 Windows；macOS/Linux 的静态与动态依赖处理见文件头） ----
    if(WIN32)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TARGET}>
                $<TARGET_FILE_DIR:${TARGET}>
            # 将 TARGET_RUNTIME_DLLS 返回的分号列表展开为多个文件参数，
            # VERBATIM 则保留每个参数的边界，避免路径中的空格被拆开。
            COMMAND_EXPAND_LISTS
            VERBATIM
            COMMENT "Copy runtime DLLs")
    endif()
endfunction()
