# LiquidMouse per Fedora (KDE Plasma 6 / Wayland)

Porting completo in **C++20 / Qt6 / KWin Effect** di [LiquidMouse](https://github.com/beniaminonardone/LiquidMouse), progettato specificamente per **Fedora Linux con KDE Plasma 6 su Wayland**.

---

## 🎯 Architettura del Porting

LiquidMouse su macOS si appoggiava a chiamate private di WindowServer e finestre trasparenti sopra lo schermo. Su **Wayland**, il modello di sicurezza non permette ad applicazioni standard in background di sovrascrivere il cursore globale o catturare le coordinate del mouse di altre finestre.

Questo porting risolve il problema alla radice integrandolo **all'interno del compositor (KWin 6)** e fornendo un'applicazione di controllo nel **Vassoio di Sistema (System Tray)**:
1. **Fisica e Geometria fedeli al 100%:**
   - [`src/spring.h`](src/spring.h): oscillatore armonico smorzato con integrazione numerica substep.
   - [`src/liquidbody.cpp`](src/liquidbody.cpp): catena di nodi, Catmull-Rom splines per la coda fluida, outline della freccia deformabile con corner-cutting e droplet splashes al clic.
   - [`src/glyph.cpp`](src/glyph.cpp): vettori e forme geometriche per tutti i contesti del cursore (Testo, Link, Ridimensionamento, Afferra, Mirino, Divisori, ecc.).
   - [`src/liquidrenderer.cpp`](src/liquidrenderer.cpp): rendering vettoriale sub-pixel antialiased con bordo di contrasto e ombra.
2. **Integrazione Nativa KWin 6 (`src/liquidmouse_effect.cpp`):**
   - Eredita da `KWin::Effect`.
   - Nasconde in modo pulito il cursore hardware tramite `Cursors::self()->hideCursor()`.
   - Riceve in tempo reale posizione e clic del mouse (`mouseChanged`).
   - Rileva la forma richiesta dalle app (`cursorShapeChanged`) e adatta la forma liquida istantaneamente.
3. **App Vassoio di Sistema & Finestra Impostazioni (`src/tray_app.cpp`):**
   - Icona sempre visibile nel vassoio di sistema di KDE Plasma (in basso a destra accanto all'orologio).
   - Regolazione della grandezza del cursore dal 50% al 300% con slider in tempo reale e pulsanti rapidi (100%, 125%, 150%, 200%).
   - Controlli completi su lunghezza scia, velocità, elasticità e schizzi d'acqua.
   - Avvio automatico integrato all'accesso.

---

## 🚀 Requisiti e Dipendenze su Fedora

Per compilare l'effetto KWin e l'applicazione su Fedora:

```bash
sudo dnf install -y gcc-c++ cmake extra-cmake-modules qt6-qtbase-devel kwin-devel kf6-kconfig-devel kf6-kcoreaddons-devel libdrm-devel
```

---

## 📦 Installazione Rapida

È presente uno script automatico che verifica le dipendenze, compila il progetto, installa l'effetto KWin in `/usr/lib64/qt6/plugins/kwin/effects/plugins/`, installa l'eseguibile in `/usr/local/bin/liquidmouse`, registra l'avvio automatico e avvia l'icona nel vassoio:

```bash
cd LiquidMouse-KWin
./install.sh
```

---

## 🖥️ Utilizzo e Regolazione Grandezza

### 1. Dall'icona nel Vassoio di Sistema (in basso a destra)
- **Click sinistro sull'icona:** apre la finestra impostazioni con slider per ingrandire/ridurre il cursore (50% - 300%) e visualizzare l'anteprima interattiva.
- **Click destro sull'icona:** apre il menu rapido con le dimensioni predefinite (100%, 125%, 150%, 175%, 200%) e lo switch di attivazione.

### 2. Dal Menu Applicazioni (Kickoff o KRunner)
Premi il tasto `Super`/`Windows`, cerca **LiquidMouse** e premi Invio per aprire direttamente il pannello di controllo.

### 3. Da Riga di Comando
Puoi anche regolare la scala al volo tramite lo script `set-scale.sh`:
```bash
./set-scale.sh 1.25   # 25% più grande
./set-scale.sh 1.5    # 50% più grande
./set-scale.sh 2.0    # 2x (doppia dimensione)
./set-scale.sh 1.0    # 100% (dimensione originale)
```

---

## 🔄 Avvio Automatico (Autostart)
LiquidMouse è configurato per avviarsi automaticamente ad ogni accesso al desktop. Il file di autostart si trova in `~/.config/autostart/liquidmouse.desktop` e puoi abilitarlo o disabilitarlo in qualsiasi momento anche dal menu tasto destro dell'icona nel vassoio o dalle Impostazioni di Sistema di KDE (sezione *Avvio automatico*).

---

## 🗑️ Disinstallazione

Per rimuovere completamente LiquidMouse:
```bash
./uninstall.sh
```
Lo script ripristina il cursore nativo di KWin, rimuove l'avvio automatico, disattiva l'effetto e rimuove tutti i file installati.
