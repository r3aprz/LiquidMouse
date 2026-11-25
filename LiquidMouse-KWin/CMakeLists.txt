cmake_minimum_required(VERSION 3.16)
project(LiquidMouse-KWin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Load KDEInstallDirs before Qt6 so KDE_INSTALL_* paths (including KDE_INSTALL_QTPLUGINDIR)
# are initialized cleanly without GNUInstallDirs conflicts.
find_package(ECM 6.0.0 QUIET NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

if(ECM_FOUND)
    find_package(KF6Config 6.0.0 QUIET)
    find_package(KF6CoreAddons 6.0.0 QUIET)
    find_package(KWin 6.0.0 QUIET)
endif()

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
target_link_libraries(liquidmouse-app PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus)
if(TARGET KF6::ConfigCore)
    target_link_libraries(liquidmouse-app PRIVATE KF6::ConfigCore)
else()
    find_package(KF6Config 6.0.0 REQUIRED)
    target_link_libraries(liquidmouse-app PRIVATE KF6::ConfigCore)
endif()

if(KWin_FOUND)
    message(STATUS "KWin ${KWin_VERSION} found. Building KWin Effect Plugin...")

    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES KWin::kwin Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus KF6::ConfigCore KF6::CoreAddons)

    # Probe KWin Effect API across Plasma 6.0 .. 6.8+
    check_cxx_source_compiles("
        #include <kwin/effect/effect.h>
        #include <kwin/core/region.h>
        #include <kwin/core/output.h>
        class ProbeEffect : public KWin::Effect {
        public:
            bool paintScreen(const KWin::RenderTarget &rt, const KWin::RenderViewport &vp,
                             int mask, const KWin::Region &r, KWin::LogicalOutput *s) override {
                return KWin::Effect::paintScreen(rt, vp, mask, r, s);
            }
        };
        int main() { return 0; }
    " LIQUIDMOUSE_KWIN_PAINTSCREEN_BOOL_REGION_LOGICALOUTPUT)

    if(NOT LIQUIDMOUSE_KWIN_PAINTSCREEN_BOOL_REGION_LOGICALOUTPUT)
        check_cxx_source_compiles("
            #include <kwin/effect/effect.h>
            #include <kwin/core/output.h>
            #include <QRegion>
            class ProbeEffect : public KWin::Effect {
            public:
                void paintScreen(const KWin::RenderTarget &rt, const KWin::RenderViewport &vp,
                                 int mask, const QRegion &r, KWin::LogicalOutput *s) override {
                    KWin::Effect::paintScreen(rt, vp, mask, r, s);
                }
            };
            int main() { return 0; }
        " LIQUIDMOUSE_KWIN_PAINTSCREEN_VOID_QREGION_LOGICALOUTPUT)
    endif()

    check_cxx_source_compiles("
        #include <kwin/core/rect.h>
        int main() { KWin::Rect r(0, 0, 10, 10); return r.isEmpty() ? 0 : 1; }
    " LIQUIDMOUSE_KWIN_HAS_CORE_RECT)

    set(CMAKE_REQUIRED_LIBRARIES)

    set(EFFECT_SOURCES
        ${CORE_SOURCES}
        src/liquidmouse_effect.h
        src/liquidmouse_effect.cpp
    )

    add_library(kwin_liquidmouse MODULE ${EFFECT_SOURCES})
    set_target_properties(kwin_liquidmouse PROPERTIES PREFIX "")

    if(LIQUIDMOUSE_KWIN_PAINTSCREEN_BOOL_REGION_LOGICALOUTPUT)
        target_compile_definitions(kwin_liquidmouse PRIVATE LIQUIDMOUSE_KWIN_PAINTSCREEN_BOOL_REGION_LOGICALOUTPUT=1)
    elseif(LIQUIDMOUSE_KWIN_PAINTSCREEN_VOID_QREGION_LOGICALOUTPUT)
        target_compile_definitions(kwin_liquidmouse PRIVATE LIQUIDMOUSE_KWIN_PAINTSCREEN_VOID_QREGION_LOGICALOUTPUT=1)
    endif()

    if(LIQUIDMOUSE_KWIN_HAS_CORE_RECT)
        target_compile_definitions(kwin_liquidmouse PRIVATE LIQUIDMOUSE_KWIN_HAS_CORE_RECT=1)
    endif()

    if(NOT KWIN_RUNNING_VERSION)
        foreach(_qdbus_cmd qdbus-qt6 qdbus6 qdbus)
            find_program(_QDBUS_EXE ${_qdbus_cmd})
            if(_QDBUS_EXE)
                execute_process(
                    COMMAND bash -c "${_QDBUS_EXE} org.kde.KWin /KWin org.kde.KWin.supportInformation 2>/dev/null | grep 'KWin version:' | awk '{print \$3}'"
                    OUTPUT_VARIABLE DETECTED_KWIN_VER
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                if(DETECTED_KWIN_VER)
                    set(KWIN_RUNNING_VERSION ${DETECTED_KWIN_VER})
                endif()
                break()
            endif()
        endforeach()
    endif()

    if(KWIN_RUNNING_VERSION AND NOT KWIN_RUNNING_VERSION STREQUAL KWin_VERSION)
        message(WARNING "Running KWin (${KWIN_RUNNING_VERSION}) differs from installed KWin devel (${KWin_VERSION}). Using running version for immediate compatibility; rebuild after rebooting.")
        target_compile_definitions(kwin_liquidmouse PRIVATE OVERRIDE_KWIN_VERSION="${KWIN_RUNNING_VERSION}")
    endif()

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

    if(KDE_INSTALL_QTPLUGINDIR)
        install(TARGETS kwin_liquidmouse DESTINATION "${KDE_INSTALL_QTPLUGINDIR}/kwin/effects/plugins")
    else()
        install(TARGETS kwin_liquidmouse DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/plugins/kwin/effects/plugins")
    endif()
else()
    message(WARNING "KWin 6 development files (kwin-devel / KF6 / libepoxy-dev) not found. KWin Effect plugin will NOT be built, only liquidmouse-app and liquidmouse-preview.")
endif()

install(TARGETS liquidmouse-app RUNTIME DESTINATION bin)
install(TARGETS liquidmouse-preview RUNTIME DESTINATION bin)
install(FILES resources/liquidmouse.desktop DESTINATION share/applications)
install(FILES resources/liquidmouse.svg DESTINATION share/icons/hicolor/scalable/apps)
