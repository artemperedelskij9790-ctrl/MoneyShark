cmake_minimum_required(VERSION 3.16)

project(MoneyShark VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql Charts Network Xml)

set(CLIENT_SOURCES
    main.cpp
    appstyle.cpp
    logo.cpp
    connectiondialog.cpp
    logindialog.cpp
    mainwindow.cpp
    financemanager.cpp
    financewidget.cpp
    statisticswidget.cpp
    database.cpp
    networkclient.cpp
    ratesprovider.cpp
    creditcalculator.cpp
    depositcalculator.cpp
    creditdialog.cpp
)

set(CLIENT_HEADERS
    appstyle.h
    logo.h
    connectiondialog.h
    logindialog.h
    mainwindow.h
    financemanager.h
    financewidget.h
    statisticswidget.h
    database.h
    networkclient.h
    ratesprovider.h
    models.h
    creditcalculator.h
    depositcalculator.h
    creditdialog.h
)

add_executable(MoneyShark
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
)

target_link_libraries(MoneyShark PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Network
    Qt6::Xml
)

add_executable(MoneySharkServer
    servermain.cpp
    moneysharkserver.cpp
    moneysharkserver.h
)

if(WIN32)
    file(TO_CMAKE_PATH "$ENV{TEMP}" MONEYSHARK_TEMP_DIR)
    if(MONEYSHARK_TEMP_DIR)
        set_target_properties(MoneyShark PROPERTIES
            AUTOGEN_BUILD_DIR "${MONEYSHARK_TEMP_DIR}/MoneySharkAutogen/MoneyShark"
        )
        set_target_properties(MoneySharkServer PROPERTIES
            AUTOGEN_BUILD_DIR "${MONEYSHARK_TEMP_DIR}/MoneySharkAutogen/MoneySharkServer"
        )
    endif()
endif()

target_link_libraries(MoneySharkServer PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Network
)

if(WIN32)
    set_target_properties(MoneyShark PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(MoneyShark PRIVATE /W3 /utf-8)
    target_compile_options(MoneySharkServer PRIVATE /W3 /utf-8)
else()
    target_compile_options(MoneyShark PRIVATE -Wall -Wextra)
    target_compile_options(MoneySharkServer PRIVATE -Wall -Wextra)
endif()

function(copy_qsqlite_for target_name)
    if(WIN32 AND Qt6_DIR)
        set(_qsqlite_candidates
            "${Qt6_DIR}/../../../plugins/sqldrivers/qsqlite.dll"
            "${Qt6_DIR}/../../../share/qt6/plugins/sqldrivers/qsqlite.dll"
        )
        set(_qsqlite_path "")
        foreach(_candidate IN LISTS _qsqlite_candidates)
            if(EXISTS "${_candidate}")
                set(_qsqlite_path "${_candidate}")
                break()
            endif()
        endforeach()

        if(_qsqlite_path)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${target_name}>/sqldrivers"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_qsqlite_path}"
                    "$<TARGET_FILE_DIR:${target_name}>/sqldrivers/qsqlite.dll"
                COMMENT "Copying qsqlite.dll for ${target_name}"
            )
        else()
            message(WARNING "qsqlite.dll was not found; SQLite plugin will not be copied for ${target_name}")
        endif()
    endif()
endfunction()

copy_qsqlite_for(MoneyShark)
copy_qsqlite_for(MoneySharkServer)

function(copy_qt_plugin_dir_for target_name plugin_name)
    if(WIN32 AND Qt6_DIR)
        set(_plugin_candidates
            "${Qt6_DIR}/../../../plugins/${plugin_name}"
            "${Qt6_DIR}/../../../share/qt6/plugins/${plugin_name}"
        )
        set(_plugin_path "")
        foreach(_candidate IN LISTS _plugin_candidates)
            if(EXISTS "${_candidate}")
                set(_plugin_path "${_candidate}")
                break()
            endif()
        endforeach()

        if(_plugin_path)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_plugin_path}"
                    "$<TARGET_FILE_DIR:${target_name}>/${plugin_name}"
                COMMENT "Copying Qt ${plugin_name} plugins for ${target_name}"
            )
        else()
            message(WARNING "Qt plugin directory '${plugin_name}' was not found for ${target_name}")
        endif()
    endif()
endfunction()

function(copy_openssl_runtime_for target_name)
    if(WIN32 AND Qt6_DIR)
        set(_openssl_candidates
            "${Qt6_DIR}/../../../bin/libssl-3-x64.dll"
            "${Qt6_DIR}/../../../bin/libcrypto-3-x64.dll"
        )
        foreach(_candidate IN LISTS _openssl_candidates)
            if(EXISTS "${_candidate}")
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_candidate}"
                        "$<TARGET_FILE_DIR:${target_name}>"
                    COMMENT "Copying OpenSSL runtime for ${target_name}"
                )
            endif()
        endforeach()
    endif()
endfunction()

copy_qt_plugin_dir_for(MoneyShark tls)
copy_openssl_runtime_for(MoneyShark)

function(copy_mingw_runtime_for target_name)
    if(WIN32 AND Qt6_DIR)
        set(_runtime_files
            libsqlite3-0.dll
            libgcc_s_seh-1.dll
            libstdc++-6.dll
            libwinpthread-1.dll
            libb2-1.dll
            libbrotlicommon.dll
            libbrotlidec.dll
            libbz2-1.dll
            libdouble-conversion.dll
            libfreetype-6.dll
            libglib-2.0-0.dll
            libgraphite2.dll
            libharfbuzz-0.dll
            libiconv-2.dll
            libicudt78.dll
            libicuin78.dll
            libicuuc78.dll
            libintl-8.dll
            libmd4c.dll
            libpcre2-16-0.dll
            libpcre2-8-0.dll
            libpng16-16.dll
            libzstd.dll
            zlib1.dll
        )
        foreach(_runtime_file IN LISTS _runtime_files)
            set(_runtime_path "${Qt6_DIR}/../../../bin/${_runtime_file}")
            if(EXISTS "${_runtime_path}")
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_runtime_path}"
                        "$<TARGET_FILE_DIR:${target_name}>/${_runtime_file}"
                    COMMENT "Copying MinGW runtime ${_runtime_file} for ${target_name}"
                )
            endif()
        endforeach()
    endif()
endfunction()

copy_mingw_runtime_for(MoneyShark)
copy_mingw_runtime_for(MoneySharkServer)

add_custom_command(TARGET MoneyShark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:MoneyShark>/assets"
    COMMENT "Copying MoneyShark assets"
)
