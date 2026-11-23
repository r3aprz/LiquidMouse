#!/usr/bin/env bash
set -e

SCALE="${1:-1.5}"

if ! [[ "$SCALE" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    echo "Uso: $0 <fattore_scala>"
    echo "Esempio: $0 1.25  (25% più grande)"
    echo "         $0 1.5   (50% più grande)"
    echo "         $0 2.0   (doppia grandezza)"
    echo "         $0 1.0   (dimensione originale 100%)"
    exit 1
fi

DOT_RADIUS=$(awk "BEGIN {printf \"%.2f\", 6.5 * $SCALE}")
TRAIL_LENGTH=$(awk "BEGIN {printf \"%.1f\", 160.0 * $SCALE}")

kwriteconfig6 --file kwinrc --group Effect-LiquidMouse --key Scale "$SCALE"
kwriteconfig6 --file kwinrc --group Effect-LiquidMouse --key DotRadius "$DOT_RADIUS"
kwriteconfig6 --file kwinrc --group Effect-LiquidMouse --key TrailLength "$TRAIL_LENGTH"

if command -v qdbus-qt6 >/dev/null 2>&1; then
    qdbus-qt6 org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect "liquidmouse" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /KWin org.kde.KWin.reconfigure 2>/dev/null || true
elif command -v qdbus6 >/dev/null 2>&1; then
    qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect "liquidmouse" 2>/dev/null || true
    qdbus6 org.kde.KWin /KWin org.kde.KWin.reconfigure 2>/dev/null || true
elif command -v dbus-send >/dev/null 2>&1; then
    dbus-send --session --dest=org.kde.KWin /KWin org.kde.KWin.reconfigure 2>/dev/null || true
fi

echo "Grandezza LiquidMouse impostata a scala: ${SCALE}x (raggio: ${DOT_RADIUS}px, scia: ${TRAIL_LENGTH}px)."
