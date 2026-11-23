#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "============================================="
echo "   LiquidMouse per Fedora (KDE Plasma 6)    "
echo "============================================="

# Required dependencies
REQUIRED_PKGS=(
    "gcc-c++"
    "cmake"
    "extra-cmake-modules"
    "qt6-qtbase-devel"
    "kwin-devel"
    "kf6-kconfig-devel"
    "kf6-kcoreaddons-devel"
    "libdrm-devel"
)

MISSING_PKGS=()
for pkg in "${REQUIRED_PKGS[@]}"; do
    if ! rpm -q "$pkg" >/dev/null 2>&1; then
        MISSING_PKGS+=("$pkg")
    fi
done

if [ ${#MISSING_PKGS[@]} -gt 0 ]; then
    echo ""
    echo "Mancano alcuni pacchetti di sviluppo necessari per la compilazione:"
    echo "  ${MISSING_PKGS[*]}"
    echo ""
    echo "Installali eseguendo il comando:"
    echo "  sudo dnf install -y ${MISSING_PKGS[*]}"
    echo ""
    read -p "Vuoi che provi a installarli adesso con sudo dnf? (s/N) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Ss]$ ]]; then
        sudo dnf install -y "${MISSING_PKGS[@]}"
    else
        echo "Installazione interrotta. Installa prima le dipendenze richieste."
        exit 1
    fi
fi

# Build
echo ""
echo "-> Compilazione in corso..."
mkdir -p build
cd build

RUNNING_KWIN_VER=$(qdbus-qt6 org.kde.KWin /KWin org.kde.KWin.supportInformation 2>/dev/null | grep 'KWin version:' | awk '{print $3}' || true)
CMAKE_EXTRA=""
if [ -n "$RUNNING_KWIN_VER" ]; then
    echo "-> Rilevata sessione KWin attiva: versione $RUNNING_KWIN_VER"
    CMAKE_EXTRA="-DKWIN_RUNNING_VERSION=$RUNNING_KWIN_VER"
fi

cmake .. -DCMAKE_BUILD_TYPE=Release $CMAKE_EXTRA
cmake --build . -j$(nproc)

# Installation paths
SYS_PLUGIN_DIR="/usr/lib64/qt6/plugins/kwin/effects/plugins"
USER_PLUGIN_DIR="$HOME/.local/lib64/qt6/plugins/kwin/effects/plugins"
USER_BIN_DIR="$HOME/.local/bin"
USER_APP_DIR="$HOME/.local/share/applications"
USER_ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"
USER_AUTOSTART_DIR="$HOME/.config/autostart"

# Rimuovi eventuali cartelle scripted effect obsolete che interferirebbero con KWin 6
rm -rf "$HOME/.local/share/kwin/effects/liquidmouse"

mkdir -p "$USER_PLUGIN_DIR" "$USER_BIN_DIR" "$USER_APP_DIR" "$USER_ICON_DIR" "$USER_AUTOSTART_DIR"

echo ""
echo "-> Installazione dei plugin in $SYS_PLUGIN_DIR..."
if [ -f "bin/kwin_liquidmouse_v9.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v9.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v9.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v9.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v9.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v9.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v9.so"
fi

if [ -f "bin/kwin_liquidmouse_v8.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v8.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v8.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v8.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v8.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v8.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v8.so"
fi

if [ -f "bin/kwin_liquidmouse_v7.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v7.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v7.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v7.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v7.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v7.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v7.so"
fi

if [ -f "bin/kwin_liquidmouse_v6.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v6.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v6.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v6.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v6.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v6.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v6.so"
fi

if [ -f "bin/kwin_liquidmouse_v5.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v5.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v5.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v5.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v5.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v5.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v5.so"
fi

if [ -f "bin/kwin_liquidmouse_v4.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v4.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v4.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v4.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v4.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v4.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v4.so"
fi

if [ -f "bin/kwin_liquidmouse_v3.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v3.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v3.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v3.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v3.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v3.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v3.so"
fi

if [ -f "bin/kwin_liquidmouse_v2.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse_v2.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v2.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse_v2.so "$SYS_PLUGIN_DIR/kwin_liquidmouse_v2.so"
    fi
    install -m 755 bin/kwin_liquidmouse_v2.so "$USER_PLUGIN_DIR/kwin_liquidmouse_v2.so"
fi

if [ -f "bin/kwin_liquidmouse.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin_liquidmouse.so "$SYS_PLUGIN_DIR/kwin_liquidmouse.so"
    else
        sudo install -m 755 bin/kwin_liquidmouse.so "$SYS_PLUGIN_DIR/kwin_liquidmouse.so"
    fi
    install -m 755 bin/kwin_liquidmouse.so "$USER_PLUGIN_DIR/kwin_liquidmouse.so"
fi

if [ -f "bin/kwin6_effect_liquidmouse.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin6_effect_liquidmouse.so "$SYS_PLUGIN_DIR/kwin6_effect_liquidmouse.so"
    else
        sudo install -m 755 bin/kwin6_effect_liquidmouse.so "$SYS_PLUGIN_DIR/kwin6_effect_liquidmouse.so"
    fi
    install -m 755 bin/kwin6_effect_liquidmouse.so "$USER_PLUGIN_DIR/kwin6_effect_liquidmouse.so"
fi

if [ -f "bin/kwin4_effect_liquidmouse.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/kwin4_effect_liquidmouse.so "$SYS_PLUGIN_DIR/kwin4_effect_liquidmouse.so"
    else
        sudo install -m 755 bin/kwin4_effect_liquidmouse.so "$SYS_PLUGIN_DIR/kwin4_effect_liquidmouse.so"
    fi
    install -m 755 bin/kwin4_effect_liquidmouse.so "$USER_PLUGIN_DIR/kwin4_effect_liquidmouse.so"
fi

if [ -f "bin/liquidmouse.so" ]; then
    if [ -w "$SYS_PLUGIN_DIR" ]; then
        install -m 755 bin/liquidmouse.so "$SYS_PLUGIN_DIR/liquidmouse.so"
    else
        sudo install -m 755 bin/liquidmouse.so "$SYS_PLUGIN_DIR/liquidmouse.so"
    fi
    install -m 755 bin/liquidmouse.so "$USER_PLUGIN_DIR/liquidmouse.so"
fi

echo "-> Installazione dell'applicazione Vassoio di Sistema & Desktop..."
if [ -f "bin/liquidmouse-app" ]; then
    # Installa in /usr/local/bin (sempre presente in PATH per KDE e systemd)
    if [ -w "/usr/local/bin" ]; then
        install -m 755 bin/liquidmouse-app "/usr/local/bin/liquidmouse"
    else
        sudo install -m 755 bin/liquidmouse-app "/usr/local/bin/liquidmouse"
    fi
    install -m 755 bin/liquidmouse-app "$USER_BIN_DIR/liquidmouse"
    install -m 755 bin/liquidmouse-app "$USER_BIN_DIR/liquidmouse-app"
    echo "Applicazione vassoio installata in /usr/local/bin/liquidmouse e $USER_BIN_DIR/liquidmouse"
fi

if [ -f "bin/liquidmouse-preview" ]; then
    install -m 755 bin/liquidmouse-preview "$USER_BIN_DIR/liquidmouse-preview"
fi

# Icona SVG ad alta risoluzione
if [ -f "../resources/liquidmouse.svg" ]; then
    install -m 644 "../resources/liquidmouse.svg" "$USER_ICON_DIR/liquidmouse.svg"
    sudo install -m 644 "../resources/liquidmouse.svg" "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg" 2>/dev/null || true
elif [ -f "resources/liquidmouse.svg" ]; then
    install -m 644 "resources/liquidmouse.svg" "$USER_ICON_DIR/liquidmouse.svg"
    sudo install -m 644 "resources/liquidmouse.svg" "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg" 2>/dev/null || true
fi

# Desktop file per menu applicazioni (apre le impostazioni con slider)
if [ -f "../resources/liquidmouse.desktop" ]; then
    install -m 644 "../resources/liquidmouse.desktop" "$USER_APP_DIR/liquidmouse.desktop"
    sudo install -m 644 "../resources/liquidmouse.desktop" "/usr/share/applications/liquidmouse.desktop" 2>/dev/null || true
elif [ -f "resources/liquidmouse.desktop" ]; then
    install -m 644 "resources/liquidmouse.desktop" "$USER_APP_DIR/liquidmouse.desktop"
    sudo install -m 644 "resources/liquidmouse.desktop" "/usr/share/applications/liquidmouse.desktop" 2>/dev/null || true
fi

# Desktop file per autostart (avvia in background all'accesso senza aprire finestre)
cat << 'EOF' > "$USER_AUTOSTART_DIR/liquidmouse.desktop"
[Desktop Entry]
Name=LiquidMouse
GenericName=Cursore Liquido
Comment=Cursore fluido con fisica liquida per KDE Plasma
Exec=/usr/local/bin/liquidmouse
Icon=liquidmouse
Terminal=false
Type=Application
Categories=Utility;
StartupNotify=false
X-KDE-autostart-phase=2
X-KDE-autostart-after=kwin
EOF

# Aggiorna database delle applicazioni e cache icone
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$USER_APP_DIR" || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi
if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
fi

echo ""
echo "-> Attivazione dell'effetto in kwinrc..."
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v9Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v8Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v7Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v6Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v5Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v4Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v3Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouse_v2Enabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin_liquidmouseEnabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin6_effect_liquidmouseEnabled true
kwriteconfig6 --file kwinrc --group Plugins --key kwin4_effect_liquidmouseEnabled true
kwriteconfig6 --file kwinrc --group Plugins --key liquidmouseEnabled true

# Reload KWin
echo "-> Ricaricamento configurazione KWin in tempo reale..."
if command -v qdbus-qt6 >/dev/null 2>&1; then
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "shakecursor" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "liquidmouse" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin4_effect_liquidmouse" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin6_effect_liquidmouse" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v2" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v3" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v4" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v5" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v6" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v7" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v8" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v9" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /KWin org.kde.KWin.reconfigure 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects loadEffect "kwin_liquidmouse_v9" 2>/dev/null || true
    qdbus-qt6 org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect "kwin_liquidmouse_v9" 2>/dev/null || true
elif command -v qdbus6 >/dev/null 2>&1; then
    qdbus6 org.kde.KWin /Effects unloadEffect "shakecursor" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "liquidmouse" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin4_effect_liquidmouse" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin6_effect_liquidmouse" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v2" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v3" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v4" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v5" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v6" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v7" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v8" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects unloadEffect "kwin_liquidmouse_v9" 2>/dev/null || true
    qdbus6 org.kde.KWin /KWin org.kde.KWin.reconfigure 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects loadEffect "kwin_liquidmouse_v9" 2>/dev/null || true
    qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect "kwin_liquidmouse_v9" 2>/dev/null || true
fi

# Avvia l'app nel vassoio di sistema (System Tray)
pkill -f "liquidmouse-app" 2>/dev/null || true
pkill -x "liquidmouse" 2>/dev/null || true
sleep 0.5
if command -v systemd-run >/dev/null 2>&1; then
    systemd-run --user --unit=liquidmouse /usr/local/bin/liquidmouse >/dev/null 2>&1 || nohup /usr/local/bin/liquidmouse </dev/null >/dev/null 2>&1 &
elif command -v kstart6 >/dev/null 2>&1; then
    kstart6 /usr/local/bin/liquidmouse >/dev/null 2>&1 &
else
    nohup /usr/local/bin/liquidmouse </dev/null >/dev/null 2>&1 & disown
fi
echo "-> Icona LiquidMouse avviata nel Vassoio di Sistema (System Tray)!"

echo ""
echo "=========================================================="
echo " LiquidMouse è stato installato e attivato con successo!  "
echo "=========================================================="
echo ""
echo "• Trovi l'icona di LiquidMouse nel vassoio di sistema in basso a destra"
echo "  (accanto all'orologio): fai clic per aprire le impostazioni con slider"
echo "  e regolare la grandezza o la fisica in tempo reale!"
echo ""
echo "• Puoi cercarla in qualsiasi momento nel menu applicazioni come 'LiquidMouse'."
echo ""
echo "• Per disinstallarlo in qualsiasi momento:"
echo "    ./uninstall.sh"
echo ""
