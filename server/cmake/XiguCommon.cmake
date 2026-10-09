# ============================================================
#  XiguCommon.cmake — 后端公共部署规则
#  xigu_deploy(<target>):
#    1. 拷贝本服务目录的 config.ini 到输出目录
#    2. Windows/MSVC 下从 vcpkg installed 拷贝运行时 DLL（按 Debug/Release 区分）
#
#  vcpkg 的 Debug/Release DLL 命名不同：
#    - boost 动态库 Debug 名带 "-gd-"（vc145-mt-gd-x64），Release 不带
#    - 部分库 Debug 名带 "d" 后缀（hiredisd/libprotobufd/lz4d/zlibd1...）
#  因此这里维护两套一一对应的清单（顺序必须一致），按配置选择源与目标名。
# ============================================================

function(xigu_deploy TARGET)
    # ---- 1. config.ini ----
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
                "$<TARGET_FILE_DIR:${TARGET}>/config.ini"
            COMMENT "Copy config.ini -> $<TARGET_FILE_DIR:${TARGET}>")
    endif()

    # ---- 2. Windows 运行时 DLL（vcpkg 动态库部署） ----
    if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
        set(_dll_root "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

        # Release 名（vcpkg installed/<triplet>/bin 实际产物，已逐项核对）
        set(_rel_dlls
            abseil_dll.dll
            boost_atomic-vc145-mt-x64-1_90.dll
            boost_chrono-vc145-mt-x64-1_90.dll
            boost_container-vc145-mt-x64-1_90.dll
            boost_context-vc145-mt-x64-1_90.dll
            boost_date_time-vc145-mt-x64-1_90.dll
            boost_filesystem-vc145-mt-x64-1_90.dll
            boost_serialization-vc145-mt-x64-1_90.dll
            boost_thread-vc145-mt-x64-1_90.dll
            boost_wserialization-vc145-mt-x64-1_90.dll
            cares.dll
            hiredis.dll
            jsoncpp.dll
            legacy.dll
            libcrypto-3-x64.dll
            libprotobuf.dll
            libprotobuf-lite.dll
            libprotoc.dll
            libssl-3-x64.dll
            lz4.dll
            mysqlcppconnx-2-vs14.dll
            re2.dll
            zlib1.dll
            zstd.dll
        )

        # Debug 名（vcpkg installed/<triplet>/debug/bin 实际产物，与上面顺序一一对应）
        set(_dbg_dlls
            abseil_dll.dll
            boost_atomic-vc145-mt-gd-x64-1_90.dll
            boost_chrono-vc145-mt-gd-x64-1_90.dll
            boost_container-vc145-mt-gd-x64-1_90.dll
            boost_context-vc145-mt-gd-x64-1_90.dll
            boost_date_time-vc145-mt-gd-x64-1_90.dll
            boost_filesystem-vc145-mt-gd-x64-1_90.dll
            boost_serialization-vc145-mt-gd-x64-1_90.dll
            boost_thread-vc145-mt-gd-x64-1_90.dll
            boost_wserialization-vc145-mt-gd-x64-1_90.dll
            cares.dll
            hiredisd.dll
            jsoncpp.dll
            legacy.dll
            libcrypto-3-x64.dll
            libprotobufd.dll
            libprotobuf-lited.dll
            libprotocd.dll
            libssl-3-x64.dll
            lz4d.dll
            mysqlcppconnx-2-vs14.dll
            re2.dll
            zlibd1.dll
            zstd.dll
        )

        list(LENGTH _rel_dlls _n)
        math(EXPR _last "${_n}-1")
        foreach(_i RANGE 0 ${_last})
            list(GET _rel_dlls ${_i} _rel)
            list(GET _dbg_dlls ${_i} _dbg)
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dll_root}/$<$<CONFIG:Debug>:debug/bin>$<$<NOT:$<CONFIG:Debug>>:bin>/$<$<CONFIG:Debug>:${_dbg}>$<$<NOT:$<CONFIG:Debug>>:${_rel}>"
                    "$<TARGET_FILE_DIR:${TARGET}>/$<$<CONFIG:Debug>:${_dbg}>$<$<NOT:$<CONFIG:Debug>>:${_rel}>"
                COMMENT "Copy runtime DLL")
        endforeach()
    endif()
endfunction()
