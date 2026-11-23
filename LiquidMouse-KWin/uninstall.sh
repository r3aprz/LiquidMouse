#!/usr/bin/env bash
set -e

echo "-> Disattivazione di LiquidMouse da KWin..."
if command -v kwriteconfig6 >/dev/null 2>&1; then
    kwriteconfig6 --file kwinrc --group Plugins --key liquidmouseEnabled false
fi

# Ferma l'app se in esecuzione
pkill -f "liquidmouse" 2>/dev/null || true

SYS_PLUGIN_FILE="/usr/lib64/qt6/plugins/kwin/effects/plugins/liquidmouse.so"
USER_PLUGIN_FILE="$HOME/.local/lib64/qt6/plugins/kwin/effects/plugins/liquidmouse.so"
OLD_USER_PLUGIN="$HOME/.local/lib64/qt6/plugins/kwin/effects/plugins/kwin4_effect_liquidmouse.so"
METADATA_DIR="$HOME/.local/share/kwin/effects/liquidmouse"
PREVIEW_BIN="$HOME/.local/bin/liquidmouse-preview"

if [ -f "$SYS_PLUGIN_FILE" ]; then
    sudo rm -fv "$SYS_PLUGIN_FILE"
fi

if [ -f "/usr/local/bin/liquidmouse" ]; then
    sudo rm -fv "/usr/local/bin/liquidmouse"
fi

if [ -f "/usr/share/applications/liquidmouse.desktop" ]; then
    sudo rm -fv "/usr/share/applications/liquidmouse.desktop"
fi

if [ -f "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg" ]; then
    sudo rm -fv "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg"
fi

rm -fv "$HOME/.local/bin/liquidmouse" "$HOME/.local/bin/liquidmouse-app"
rm -fv "$HOME/.local/share/applications/liquidmouse.desktop"
rm -fv "$HOME/.config/autostart/liquidmouse.desktop"
rm -fv "$HOME/.local/share/icons/hicolor/scalable/apps/liquidmouse.svg"

if [ -f "$USER_PLUGIN_FILE" ]; then
    rm -fv "$USER_PLUGIN_FILE"
fi

if [ -f "$OLD_USER_PLUGIN" ]; then
    rm -fv "$OLD_USER_PLUGIN"
fi

if [ -f "$PREVIEW_BIN" ]; then
    rm -fv "$PREVIEW_BIN"
fi

if [ -d "$METADATA_DIR" ]; then
    rm -rfv "$METADATA_DIR"
fi

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
fi
if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 --noincremental 2>/dev/null || true
fi

echo "-> Ripristino cursore e ricaricamento KWin..."
if command -v qdbus-qt6 >/dev/null 2>&1; then
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "liquidmouse" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /KWin reconfigure 2>/dev/null || true
elif command -v qdbus6 >/dev/null 2>&1; then
    qdbus6 org.kde.KWin /Effects unloadEffect "liquidmouse" 2>/dev/null || true
    qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || true
fi

echo "LiquidMouse è stato disinstallato con successo."
