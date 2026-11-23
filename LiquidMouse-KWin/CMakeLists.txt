cmake_minimum_required(VERSION 3.16)
project(LiquidMouse-KWin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

# Common physics and rendering core sources
set(CORE_SOURCES
    src/spring.h
    src/cursorcontext.h
    src/glyph.h
    src/glyph.cpp
    src/liquidbody.h
    src/liquidbody.cpp
    src/liquidrenderer.h
    src/liquidrenderer.cpp
)

# Standalone preview & test harness application
add_executable(liquidmouse-preview
    ${CORE_SOURCES}
    src/preview_main.cpp
)
target_link_libraries(liquidmouse-preview PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# Tray & Configuration GUI Application
add_executable(liquidmouse-app
    ${CORE_SOURCES}
    src/tray_app.cpp
)
target_link_libraries(liquidmouse-app PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus KF6::ConfigCore)

# Check for KWin & KDE Frameworks 6 for the compositor effect plugin
find_package(ECM 6.0.0 QUIET NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)

    find_package(KF6Config 6.0.0 QUIET)
    find_package(KF6CoreAddons 6.0.0 QUIET)
    find_package(KWin 6.0.0 QUIET)
endif()

if(KWin_FOUND)
    message(STATUS "KWin 6 found. Building KWin Effect Plugin...")

    set(EFFECT_SOURCES
        ${CORE_SOURCES}
        src/liquidmouse_effect.h
        src/liquidmouse_effect.cpp
    )

    add_library(liquidmouse MODULE ${EFFECT_SOURCES})
    set_target_properties(liquidmouse PROPERTIES PREFIX "")

    add_library(kwin4_effect_liquidmouse MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin4_effect_liquidmouse PROPERTIES PREFIX "")

    add_library(kwin6_effect_liquidmouse MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin6_effect_liquidmouse PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v2 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v2 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v3 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v3 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v4 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v4 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v5 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v5 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v6 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v6 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v7 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v7 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v8 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v8 PROPERTIES PREFIX "")

    add_library(kwin_liquidmouse_v9 MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse_v9 PROPERTIES PREFIX "")

    if(NOT KWIN_RUNNING_VERSION)
        execute_process(
            COMMAND bash -c "qdbus-qt6 org.kde.KWin /KWin org.kde.KWin.supportInformation 2>/dev/null | grep 'KWin version:' | awk '{print \$3}'"
            OUTPUT_VARIABLE DETECTED_KWIN_VER
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(DETECTED_KWIN_VER)
            set(KWIN_RUNNING_VERSION ${DETECTED_KWIN_VER})
        endif()
    endif()

    if(KWIN_RUNNING_VERSION)
        message(STATUS "Targeting running KWin version: ${KWIN_RUNNING_VERSION}")
        target_compile_definitions(liquidmouse PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin4_effect_liquidmouse PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin6_effect_liquidmouse PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v2 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v3 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v4 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v5 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v6 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v7 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v8 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
        target_compile_definitions(kwin_liquidmouse_v9 PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
    endif()

    target_link_libraries(liquidmouse
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin4_effect_liquidmouse
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin6_effect_liquidmouse
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v2
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v3
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v4
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v5
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v6
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v7
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v8
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    target_link_libraries(kwin_liquidmouse_v9
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::DBus
            KF6::ConfigCore
            KF6::CoreAddons
            KWin::kwin
    )

    install(TARGETS liquidmouse kwin4_effect_liquidmouse kwin6_effect_liquidmouse kwin_liquidmouse kwin_liquidmouse_v2 kwin_liquidmouse_v3 kwin_liquidmouse_v4 kwin_liquidmouse_v5 kwin_liquidmouse_v6 kwin_liquidmouse_v7 kwin_liquidmouse_v8 kwin_liquidmouse_v9 DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/kwin/effects/plugins)
else()
    message(WARNING "KWin 6 development files (kwin-devel / KF6) not found. KWin Effect plugin will NOT be built, only liquidmouse-preview. Install kwin-devel and kf6-kconfig-devel to build the native KWin effect.")
endif()

install(TARGETS liquidmouse-app RUNTIME DESTINATION bin)
install(TARGETS liquidmouse-preview RUNTIME DESTINATION bin)
install(FILES resources/liquidmouse.desktop DESTINATION share/applications)
install(FILES resources/liquidmouse.svg DESTINATION share/icons/hicolor/scalable/apps)
