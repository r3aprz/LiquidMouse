#!/usr/bin/env bash
set -e

find_cmd() {
    for cmd in "$@"; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    return 1
}

QDBUS_BIN=$(find_cmd qdbus-qt6 qdbus6 qdbus || true)
QTPATHS_BIN=$(find_cmd qtpaths6 qtpaths-qt6 qtpaths || true)
KWRITECONFIG_BIN=$(find_cmd kwriteconfig6 kwriteconfig5 || true)
KBUILDSYCOCA_BIN=$(find_cmd kbuildsycoca6 kbuildsycoca5 || true)

PLUGIN_NAMES=(
    "kwin_liquidmouse"
    "kwin_liquidmouse_v9"
    "kwin_liquidmouse_v8"
    "kwin_liquidmouse_v7"
    "kwin_liquidmouse_v6"
    "kwin_liquidmouse_v5"
    "kwin_liquidmouse_v4"
    "kwin_liquidmouse_v3"
    "kwin_liquidmouse_v2"
    "kwin6_effect_liquidmouse"
    "kwin4_effect_liquidmouse"
    "liquidmouse"
)

echo "-> Disattivazione di LiquidMouse da KWin..."
if [ -n "$KWRITECONFIG_BIN" ]; then
    for eff in "${PLUGIN_NAMES[@]}"; do
        "$KWRITECONFIG_BIN" --file kwinrc --group Plugins --key "${eff}Enabled" false
    done
fi

# Ferma l'app se in esecuzione
pkill -f "liquidmouse-app" 2>/dev/null || true
pkill -x "liquidmouse" 2>/dev/null || true

QT_PLUGIN_ROOT=""
if [ -n "$QTPATHS_BIN" ]; then
    QT_PLUGIN_ROOT=$("$QTPATHS_BIN" --plugin-dir 2>/dev/null || "$QTPATHS_BIN" --query QT_INSTALL_PLUGINS 2>/dev/null || true)
fi
if [ -z "$QT_PLUGIN_ROOT" ] && command -v qmake6 >/dev/null 2>&1; then
    QT_PLUGIN_ROOT=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || true)
fi

PLUGIN_DIRS=(
    "/usr/lib64/qt6/plugins/kwin/effects/plugins"
    "/usr/lib/qt6/plugins/kwin/effects/plugins"
    "$HOME/.local/lib64/qt6/plugins/kwin/effects/plugins"
    "$HOME/.local/lib/qt6/plugins/kwin/effects/plugins"
)
if [ -n "$QT_PLUGIN_ROOT" ]; then
    PLUGIN_DIRS=("$QT_PLUGIN_ROOT/kwin/effects/plugins" "${PLUGIN_DIRS[@]}")
fi

for dir in "${PLUGIN_DIRS[@]}"; do
    for eff in "${PLUGIN_NAMES[@]}"; do
        if [ -f "$dir/${eff}.so" ]; then
            if [ -w "$dir" ]; then
                rm -fv "$dir/${eff}.so"
            else
                sudo rm -fv "$dir/${eff}.so"
            fi
        fi
    done
done

for sysfile in "/usr/local/bin/liquidmouse" "/usr/share/applications/liquidmouse.desktop" "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg"; do
    if [ -f "$sysfile" ]; then
        sudo rm -fv "$sysfile"
    fi
done

rm -fv "$HOME/.local/bin/liquidmouse" "$HOME/.local/bin/liquidmouse-app" "$HOME/.local/bin/liquidmouse-preview"
rm -fv "$HOME/.local/share/applications/liquidmouse.desktop"
rm -fv "$HOME/.config/autostart/liquidmouse.desktop"
rm -fv "$HOME/.local/share/icons/hicolor/scalable/apps/liquidmouse.svg"
rm -rfv "$HOME/.local/share/kwin/effects/liquidmouse"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
fi
if [ -n "$KBUILDSYCOCA_BIN" ]; then
    "$KBUILDSYCOCA_BIN" --noincremental 2>/dev/null || true
fi

echo "-> Ripristino cursore e ricaricamento KWin..."
if [ -n "$QDBUS_BIN" ]; then
    for eff in "${PLUGIN_NAMES[@]}"; do
        "$QDBUS_BIN" org.kde.KWin /Effects unloadEffect "$eff" 2>/dev/null || true
    done
    "$QDBUS_BIN" org.kde.KWin /KWin reconfigure 2>/dev/null || true
fi

echo "LiquidMouse è stato disinstallato con successo."
