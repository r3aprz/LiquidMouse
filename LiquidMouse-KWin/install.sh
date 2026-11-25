#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

if [ "${EUID:-$(id -u)}" -eq 0 ]; then
    echo "ERRORE: Non eseguire install.sh con sudo o come root."
    echo "Eseguilo come utente normale (./install.sh): chiederà sudo solo quando necessario."
    exit 1
fi

echo "============================================="
echo "  LiquidMouse per KDE Plasma 6 (Linux/Wayland)"
echo "============================================="

# Helper to find the first available command from a list
find_cmd() {
    for cmd in "$@"; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    return 1
}

# Check / install build dependencies depending on distro package manager
check_dependencies() {
    local missing=()

    if command -v dnf >/dev/null 2>&1 && command -v rpm >/dev/null 2>&1; then
        local pkgs=(
            "gcc-c++" "cmake" "extra-cmake-modules"
            "qt6-qtbase-devel" "qt6-qtdeclarative-devel"
            "kwin-devel" "kf6-kconfig-devel" "kf6-kcoreaddons-devel" "kf6-kwindowsystem-devel"
            "libdrm-devel" "libepoxy-devel" "wayland-devel"
            "vulkan-loader-devel" "vulkan-headers"
        )
        for pkg in "${pkgs[@]}"; do
            rpm -q "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
        done
        if [ ${#missing[@]} -gt 0 ]; then
            echo ""
            echo "Mancano alcuni pacchetti di sviluppo: ${missing[*]}"
            read -p "Vuoi installarli con 'sudo dnf install -y ${missing[*]}'? (s/N) " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[SsYy]$ ]]; then
                sudo dnf install -y "${missing[@]}"
            else
                echo "Installazione interrotta."
                exit 1
            fi
        fi
    elif command -v pacman >/dev/null 2>&1; then
        local pkgs=(
            "base-devel" "cmake" "extra-cmake-modules"
            "qt6-base" "qt6-declarative"
            "kwin" "kconfig" "kcoreaddons" "kwindowsystem"
            "libdrm" "libepoxy" "wayland" "vulkan-icd-loader" "vulkan-headers"
        )
        for pkg in "${pkgs[@]}"; do
            pacman -Qi "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
        done
        if [ ${#missing[@]} -gt 0 ]; then
            echo ""
            echo "Mancano alcuni pacchetti: ${missing[*]}"
            read -p "Vuoi installarli con 'sudo pacman -S --needed ${missing[*]}'? (s/N) " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[SsYy]$ ]]; then
                sudo pacman -S --needed "${missing[@]}"
            else
                echo "Installazione interrotta."
                exit 1
            fi
        fi
    elif command -v apt-get >/dev/null 2>&1 && command -v dpkg >/dev/null 2>&1; then
        local pkgs=(
            "g++" "cmake" "extra-cmake-modules"
            "qt6-base-dev" "qt6-declarative-dev"
            "kwin-dev" "libkf6config-dev" "libkf6coreaddons-dev" "libkf6windowsystem-dev"
            "libdrm-dev" "libepoxy-dev" "libwayland-dev" "libvulkan-dev"
        )
        for pkg in "${pkgs[@]}"; do
            dpkg -s "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
        done
        if [ ${#missing[@]} -gt 0 ]; then
            echo ""
            echo "Mancano alcuni pacchetti: ${missing[*]}"
            read -p "Vuoi installarli con 'sudo apt-get install -y ${missing[*]}'? (s/N) " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[SsYy]$ ]]; then
                sudo apt-get update && sudo apt-get install -y "${missing[@]}"
            else
                echo "Installazione interrotta."
                exit 1
            fi
        fi
    elif command -v zypper >/dev/null 2>&1 && command -v rpm >/dev/null 2>&1; then
        local pkgs=(
            "gcc-c++" "cmake" "extra-cmake-modules"
            "qt6-base-devel" "qt6-declarative-devel"
            "kwin6-devel" "kf6-kconfig-devel" "kf6-kcoreaddons-devel" "kf6-kwindowsystem-devel"
            "libdrm-devel" "libepoxy-devel" "wayland-devel" "vulkan-devel"
        )
        for pkg in "${pkgs[@]}"; do
            rpm -q "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
        done
        if [ ${#missing[@]} -gt 0 ]; then
            echo ""
            echo "Mancano alcuni pacchetti: ${missing[*]}"
            read -p "Vuoi installarli con 'sudo zypper install -y ${missing[*]}'? (s/N) " -n 1 -r
            echo ""
            if [[ $REPLY =~ ^[SsYy]$ ]]; then
                sudo zypper install -y "${missing[@]}"
            else
                echo "Installazione interrotta."
                exit 1
            fi
        fi
    else
        for tool in cmake g++; do
            if ! command -v "$tool" >/dev/null 2>&1; then
                echo "ERRORE: '$tool' non trovato. Installa cmake, compilatore C++, Qt6 e KWin devel."
                exit 1
            fi
        done
    fi
}

check_dependencies

QDBUS_BIN=$(find_cmd qdbus-qt6 qdbus6 qdbus || true)
QTPATHS_BIN=$(find_cmd qtpaths6 qtpaths-qt6 qtpaths || true)
KWRITECONFIG_BIN=$(find_cmd kwriteconfig6 kwriteconfig5 || true)
KBUILDSYCOCA_BIN=$(find_cmd kbuildsycoca6 kbuildsycoca5 || true)
KSTART_BIN=$(find_cmd kstart6 kstart kstart5 || true)

# Build
echo ""
echo "-> Compilazione in corso..."
mkdir -p build
cd build

# Se la cache precedente è stata creata da root o senza KDE_INSTALL_QTPLUGINDIR, rigenerala pulita
if [ -f "CMakeCache.txt" ] && [ ! -w "CMakeCache.txt" ]; then
    sudo rm -rf "$DIR/build"
    mkdir -p "$DIR/build"
    cd "$DIR/build"
fi

RUNNING_KWIN_VER=""
if [ -n "$QDBUS_BIN" ]; then
    RUNNING_KWIN_VER=$("$QDBUS_BIN" org.kde.KWin /KWin org.kde.KWin.supportInformation 2>/dev/null | grep 'KWin version:' | awk '{print $3}' || true)
fi

CMAKE_EXTRA=()
if [ -n "$RUNNING_KWIN_VER" ]; then
    echo "-> Rilevata sessione KWin attiva: versione $RUNNING_KWIN_VER"
    CMAKE_EXTRA+=("-DKWIN_RUNNING_VERSION=$RUNNING_KWIN_VER")
fi

cmake .. -DCMAKE_BUILD_TYPE=Release "${CMAKE_EXTRA[@]}"
cmake --build . -j"$(nproc)"

# Detect Qt6 plugin directory dynamically for any distro / Plasma 6 setup
QT_PLUGIN_ROOT=""
if [ -n "$QTPATHS_BIN" ]; then
    QT_PLUGIN_ROOT=$("$QTPATHS_BIN" --plugin-dir 2>/dev/null || "$QTPATHS_BIN" --query QT_INSTALL_PLUGINS 2>/dev/null || true)
fi
if [ -z "$QT_PLUGIN_ROOT" ] && command -v qmake6 >/dev/null 2>&1; then
    QT_PLUGIN_ROOT=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || true)
fi
if [ -z "$QT_PLUGIN_ROOT" ] && [ -f "CMakeCache.txt" ]; then
    REL_QTPLUGINDIR=$(grep '^KDE_INSTALL_QTPLUGINDIR:PATH=' CMakeCache.txt | cut -d= -f2- || true)
    if [ -n "$REL_QTPLUGINDIR" ]; then
        QT_PLUGIN_ROOT="/usr/$REL_QTPLUGINDIR"
    fi
fi
if [ -z "$QT_PLUGIN_ROOT" ]; then
    if [ -d "/usr/lib64/qt6/plugins" ]; then
        QT_PLUGIN_ROOT="/usr/lib64/qt6/plugins"
    else
        QT_PLUGIN_ROOT="/usr/lib/qt6/plugins"
    fi
fi

# Derive matching relative subpath (e.g. lib64/qt6/plugins or lib/qt6/plugins or lib/x86_64-linux-gnu/qt6/plugins)
REL_PLUGIN_SUBPATH="${QT_PLUGIN_ROOT#/usr/}"
if [ "$REL_PLUGIN_SUBPATH" = "$QT_PLUGIN_ROOT" ]; then
    REL_PLUGIN_SUBPATH="lib/qt6/plugins"
fi

SYS_PLUGIN_DIR="$QT_PLUGIN_ROOT/kwin/effects/plugins"
USER_PLUGIN_DIR="$HOME/.local/$REL_PLUGIN_SUBPATH/kwin/effects/plugins"
USER_BIN_DIR="$HOME/.local/bin"
USER_APP_DIR="$HOME/.local/share/applications"
USER_ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"
USER_AUTOSTART_DIR="$HOME/.config/autostart"

# Rimuovi eventuali cartelle scripted effect obsolete che interferirebbero con KWin 6
rm -rf "$HOME/.local/share/kwin/effects/liquidmouse"

mkdir -p "$USER_PLUGIN_DIR" "$USER_BIN_DIR" "$USER_APP_DIR" "$USER_ICON_DIR" "$USER_AUTOSTART_DIR"

echo ""
echo "-> Installazione del plugin in $SYS_PLUGIN_DIR..."
if [ ! -f "bin/kwin_liquidmouse.so" ]; then
    echo "ERRORE: Il plugin KWin (bin/kwin_liquidmouse.so) non è stato compilato."
    echo "Controlla l'output di CMake qui sopra per eventuali pacchetti *-devel/*-dev mancanti."
    exit 1
fi

LEGACY_PLUGIN_NAMES=(
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

for old_name in "${LEGACY_PLUGIN_NAMES[@]}"; do
    rm -f "$USER_PLUGIN_DIR/${old_name}.so"
    rm -f "$HOME/.local/lib/qt6/plugins/kwin/effects/plugins/${old_name}.so" \
          "$HOME/.local/lib64/qt6/plugins/kwin/effects/plugins/${old_name}.so"
    if [ -f "$SYS_PLUGIN_DIR/${old_name}.so" ]; then
        if [ -w "$SYS_PLUGIN_DIR" ]; then
            rm -f "$SYS_PLUGIN_DIR/${old_name}.so"
        else
            sudo rm -f "$SYS_PLUGIN_DIR/${old_name}.so"
        fi
    fi
done

if [ ! -d "$SYS_PLUGIN_DIR" ] || [ ! -w "$SYS_PLUGIN_DIR" ]; then
    sudo mkdir -p "$SYS_PLUGIN_DIR"
fi

if [ -w "$SYS_PLUGIN_DIR" ]; then
    install -D -m 755 bin/kwin_liquidmouse.so "$SYS_PLUGIN_DIR/kwin_liquidmouse.so"
else
    sudo install -D -m 755 bin/kwin_liquidmouse.so "$SYS_PLUGIN_DIR/kwin_liquidmouse.so"
fi
install -D -m 755 bin/kwin_liquidmouse.so "$USER_PLUGIN_DIR/kwin_liquidmouse.so"

echo "-> Installazione dell'applicazione Vassoio di Sistema & Desktop..."
if [ -f "bin/liquidmouse-app" ]; then
    if [ -w "/usr/local/bin" ]; then
        install -D -m 755 bin/liquidmouse-app "/usr/local/bin/liquidmouse"
    else
        sudo install -D -m 755 bin/liquidmouse-app "/usr/local/bin/liquidmouse"
    fi
    install -D -m 755 bin/liquidmouse-app "$USER_BIN_DIR/liquidmouse"
    install -D -m 755 bin/liquidmouse-app "$USER_BIN_DIR/liquidmouse-app"
    echo "Applicazione vassoio installata in /usr/local/bin/liquidmouse e $USER_BIN_DIR/liquidmouse"
fi

if [ -f "bin/liquidmouse-preview" ]; then
    install -D -m 755 bin/liquidmouse-preview "$USER_BIN_DIR/liquidmouse-preview"
fi

# Icona SVG ad alta risoluzione
if [ -f "../resources/liquidmouse.svg" ]; then
    install -D -m 644 "../resources/liquidmouse.svg" "$USER_ICON_DIR/liquidmouse.svg"
    sudo install -D -m 644 "../resources/liquidmouse.svg" "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg" 2>/dev/null || true
elif [ -f "resources/liquidmouse.svg" ]; then
    install -D -m 644 "resources/liquidmouse.svg" "$USER_ICON_DIR/liquidmouse.svg"
    sudo install -D -m 644 "resources/liquidmouse.svg" "/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg" 2>/dev/null || true
fi

# Desktop file per menu applicazioni (apre le impostazioni con slider)
if [ -f "../resources/liquidmouse.desktop" ]; then
    install -D -m 644 "../resources/liquidmouse.desktop" "$USER_APP_DIR/liquidmouse.desktop"
    sudo install -D -m 644 "../resources/liquidmouse.desktop" "/usr/share/applications/liquidmouse.desktop" 2>/dev/null || true
elif [ -f "resources/liquidmouse.desktop" ]; then
    install -D -m 644 "resources/liquidmouse.desktop" "$USER_APP_DIR/liquidmouse.desktop"
    sudo install -D -m 644 "resources/liquidmouse.desktop" "/usr/share/applications/liquidmouse.desktop" 2>/dev/null || true
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
if [ -n "$KBUILDSYCOCA_BIN" ]; then
    "$KBUILDSYCOCA_BIN" --noincremental >/dev/null 2>&1 || true
fi

echo ""
echo "-> Attivazione dell'effetto in kwinrc..."
if [ -n "$KWRITECONFIG_BIN" ]; then
    "$KWRITECONFIG_BIN" --file kwinrc --group Plugins --key kwin_liquidmouseEnabled true
    for old_name in "${LEGACY_PLUGIN_NAMES[@]}"; do
        "$KWRITECONFIG_BIN" --file kwinrc --group Plugins --key "${old_name}Enabled" false
    done
fi

# Reload KWin
echo "-> Ricaricamento configurazione KWin in tempo reale..."
if [ -n "$QDBUS_BIN" ]; then
    "$QDBUS_BIN" org.kde.KWin /Effects unloadEffect "shakecursor" 2>/dev/null || true
    for old_name in "${LEGACY_PLUGIN_NAMES[@]}"; do
        "$QDBUS_BIN" org.kde.KWin /Effects unloadEffect "$old_name" 2>/dev/null || true
    done
    "$QDBUS_BIN" org.kde.KWin /Effects unloadEffect "kwin_liquidmouse" 2>/dev/null || true
    "$QDBUS_BIN" org.kde.KWin /KWin org.kde.KWin.reconfigure 2>/dev/null || true
    "$QDBUS_BIN" org.kde.KWin /Effects loadEffect "kwin_liquidmouse" 2>/dev/null || true
    "$QDBUS_BIN" org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect "kwin_liquidmouse" 2>/dev/null || true
fi

# Avvia l'app nel vassoio di sistema (System Tray)
pkill -f "liquidmouse-app" 2>/dev/null || true
pkill -x "liquidmouse" 2>/dev/null || true
sleep 0.5
if command -v systemd-run >/dev/null 2>&1; then
    systemd-run --user --unit=liquidmouse /usr/local/bin/liquidmouse >/dev/null 2>&1 || nohup /usr/local/bin/liquidmouse </dev/null >/dev/null 2>&1 &
elif [ -n "$KSTART_BIN" ]; then
    "$KSTART_BIN" /usr/local/bin/liquidmouse >/dev/null 2>&1 &
else
    nohup /usr/local/bin/liquidmouse </dev/null >/dev/null 2>&1 & disown
fi
echo "-> Icona LiquidMouse avviata nel Vassoio di Sistema (System Tray)!"

echo ""
echo "=========================================================="
echo " LiquidMouse è stato installato e attivato con successo!  "
echo "=========================================================="
echo ""
echo "• Plugin KWin: $SYS_PLUGIN_DIR/kwin_liquidmouse.so"
echo "• Trovi l'icona di LiquidMouse nel vassoio di sistema in basso a destra."
echo "• Puoi cercarla in qualsiasi momento nel menu applicazioni come 'LiquidMouse'."
echo "• Per disinstallarlo in qualsiasi momento: ./uninstall.sh"
echo ""
