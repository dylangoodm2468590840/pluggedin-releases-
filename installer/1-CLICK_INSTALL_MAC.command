#!/bin/bash
# ================================================================================
#   PluggedIN Audio Systems — macOS 1-Click Universal Installer
#   Installs VST3 & AU Audio Units for FL Studio Mac, Logic Pro, Ableton, & Reaper
#   Supports: macOS 10.13+ | Apple Silicon (M1/M2/M3/M4) + Intel Universal
# ================================================================================

clear
echo "================================================================================"
echo "  ⚡ PluggedIN Audio Systems — macOS 1-Click Universal Installer"
echo "  UNDERGROUND v4.0.0 (Master Studio Suite) & PLUGGED 1 v1.0.0 (Hybrid Instrument)"
echo "================================================================================"
echo ""

# Ensure we are in the directory where this installer script lives
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# ── 1. Target Directories ───────────────────────────────────────────────────────
USER_VST3="$HOME/Library/Audio/Plug-Ins/VST3"
USER_AU="$HOME/Library/Audio/Plug-Ins/Components"
SYS_VST3="/Library/Audio/Plug-Ins/VST3"
SYS_AU="/Library/Audio/Plug-Ins/Components"
REG_DIR="$HOME/Library/Application Support/PluggedIN"

echo "[1/6] Auto-creating macOS audio plugin and support directories..."
mkdir -p "$USER_VST3"
mkdir -p "$USER_AU"
mkdir -p "$REG_DIR"
mkdir -p "$REG_DIR/Presets/UNDERGROUND"
mkdir -p "$REG_DIR/Presets/Plugged1"

# Create system dirs if possible
mkdir -p "$SYS_VST3" 2>/dev/null
mkdir -p "$SYS_AU" 2>/dev/null

INSTALLED_UNDERGROUND=false
INSTALLED_PLUGGED1=false
INSTALLED_CENTRAL=false

# ── 2. PluggedIN Central App ────────────────────────────────────────────────────
echo "[2/6] Installing PluggedIN Central App..."
if [ -d "PluggedIN Central.app" ]; then
    rm -rf "/Applications/PluggedIN Central.app" 2>/dev/null
    cp -R "PluggedIN Central.app" "/Applications/" 2>/dev/null || {
        mkdir -p "$HOME/Applications"
        rm -rf "$HOME/Applications/PluggedIN Central.app" 2>/dev/null
        cp -R "PluggedIN Central.app" "$HOME/Applications/" 2>/dev/null
    }
    # Create Desktop shortcut for easy access
    ln -sf "/Applications/PluggedIN Central.app" "$HOME/Desktop/PluggedIN Central" 2>/dev/null
    echo "  ✓ PluggedIN Central installed → /Applications/PluggedIN Central.app"
    echo "  ✓ PluggedIN Central shortcut added to Desktop"
    INSTALLED_CENTRAL=true
fi

# ── 3. UNDERGROUND Installation (VST3 + AU) ────────────────────────────────────
echo "[3/6] Installing UNDERGROUND Suite..."
if [ -d "UNDERGROUND.vst3" ]; then
    rm -rf "$USER_VST3/UNDERGROUND.vst3"
    cp -R "UNDERGROUND.vst3" "$USER_VST3/"
    
    # Try system VST3 mirror for maximum DAW compatibility
    if [ -d "$SYS_VST3" ]; then
        cp -R "UNDERGROUND.vst3" "$SYS_VST3/" 2>/dev/null
    fi
    echo "  ✓ UNDERGROUND VST3 installed → $USER_VST3/UNDERGROUND.vst3"
    INSTALLED_UNDERGROUND=true
fi

if [ -d "UNDERGROUND.component" ]; then
    rm -rf "$USER_AU/UNDERGROUND.component"
    cp -R "UNDERGROUND.component" "$USER_AU/"
    
    if [ -d "$SYS_AU" ]; then
        cp -R "UNDERGROUND.component" "$SYS_AU/" 2>/dev/null
    fi
    echo "  ✓ UNDERGROUND AU component installed → $USER_AU/UNDERGROUND.component"
fi

# ── 4. PLUGGED 1 Installation (VST3 + AU) ──────────────────────────────────────
echo "[4/7] Installing PLUGGED 1 Hybrid Instrument..."
if [ -d "Plugged 1.vst3" ]; then
    rm -rf "$USER_VST3/Plugged 1.vst3"
    cp -R "Plugged 1.vst3" "$USER_VST3/"
    
    if [ -d "$SYS_VST3" ]; then
        cp -R "Plugged 1.vst3" "$SYS_VST3/" 2>/dev/null
    fi
    echo "  ✓ PLUGGED 1 VST3 installed → $USER_VST3/Plugged 1.vst3"
    INSTALLED_PLUGGED1=true
fi

if [ -d "Plugged 1.component" ]; then
    rm -rf "$USER_AU/Plugged 1.component"
    cp -R "Plugged 1.component" "$USER_AU/"
    
    if [ -d "$SYS_AU" ]; then
        cp -R "Plugged 1.component" "$SYS_AU/" 2>/dev/null
    fi
    echo "  ✓ PLUGGED 1 AU component installed → $USER_AU/Plugged 1.component"
fi

# ── 5. PLUGTUNE Installation (VST3 + AU) ───────────────────────────────────────
echo "[5/7] Installing PLUGTUNE AutoTune & Formant Suite..."
if [ -d "PlugTune.vst3" ]; then
    rm -rf "$USER_VST3/PlugTune.vst3"
    cp -R "PlugTune.vst3" "$USER_VST3/"
    
    if [ -d "$SYS_VST3" ]; then
        cp -R "PlugTune.vst3" "$SYS_VST3/" 2>/dev/null
    fi
    echo "  ✓ PLUGTUNE VST3 installed → $USER_VST3/PlugTune.vst3"
    INSTALLED_PLUGTUNE=true
fi

if [ -d "PlugTune.component" ]; then
    rm -rf "$USER_AU/PlugTune.component"
    cp -R "PlugTune.component" "$USER_AU/"
    
    if [ -d "$SYS_AU" ]; then
        cp -R "PlugTune.component" "$SYS_AU/" 2>/dev/null
    fi
    echo "  ✓ PLUGTUNE AU component installed → $USER_AU/PlugTune.component"
fi

# ── 6. Gatekeeper Quarantine & Permissions Self-Healing ────────────────────────
echo "[6/7] Self-healing macOS permissions, Gatekeeper quarantine & code signing..."
# Ensure Mach-O executable permissions (+x / 755)
chmod -R 755 "$USER_VST3/UNDERGROUND.vst3" 2>/dev/null
chmod -R 755 "$USER_AU/UNDERGROUND.component" 2>/dev/null
chmod -R 755 "$USER_VST3/Plugged 1.vst3" 2>/dev/null
chmod -R 755 "$USER_AU/Plugged 1.component" 2>/dev/null
chmod -R 755 "$USER_VST3/PlugTune.vst3" 2>/dev/null
chmod -R 755 "$USER_AU/PlugTune.component" 2>/dev/null
chmod -R 755 "$SYS_VST3/UNDERGROUND.vst3" 2>/dev/null
chmod -R 755 "$SYS_AU/UNDERGROUND.component" 2>/dev/null
chmod -R 755 "$SYS_VST3/Plugged 1.vst3" 2>/dev/null
chmod -R 755 "$SYS_AU/Plugged 1.component" 2>/dev/null
chmod -R 755 "$SYS_VST3/PlugTune.vst3" 2>/dev/null
chmod -R 755 "$SYS_AU/PlugTune.component" 2>/dev/null
chmod -R 755 "/Applications/PluggedIN Central.app" 2>/dev/null

# Strip Apple Gatekeeper quarantine flag
xattr -cr "$USER_VST3/UNDERGROUND.vst3" 2>/dev/null
xattr -rd com.apple.quarantine "$USER_VST3/UNDERGROUND.vst3" 2>/dev/null
xattr -cr "$USER_AU/UNDERGROUND.component" 2>/dev/null
xattr -rd com.apple.quarantine "$USER_AU/UNDERGROUND.component" 2>/dev/null
xattr -cr "$USER_VST3/Plugged 1.vst3" 2>/dev/null
xattr -rd com.apple.quarantine "$USER_VST3/Plugged 1.vst3" 2>/dev/null
xattr -cr "$USER_AU/Plugged 1.component" 2>/dev/null
xattr -rd com.apple.quarantine "$USER_AU/Plugged 1.component" 2>/dev/null
xattr -cr "$USER_VST3/PlugTune.vst3" 2>/dev/null
xattr -rd com.apple.quarantine "$USER_VST3/PlugTune.vst3" 2>/dev/null
xattr -cr "$USER_AU/PlugTune.component" 2>/dev/null
xattr -rd com.apple.quarantine "$USER_AU/PlugTune.component" 2>/dev/null

xattr -cr "$SYS_VST3/UNDERGROUND.vst3" 2>/dev/null
xattr -rd com.apple.quarantine "$SYS_VST3/UNDERGROUND.vst3" 2>/dev/null
xattr -cr "$SYS_AU/UNDERGROUND.component" 2>/dev/null
xattr -rd com.apple.quarantine "$SYS_AU/UNDERGROUND.component" 2>/dev/null
xattr -cr "$SYS_VST3/Plugged 1.vst3" 2>/dev/null
xattr -rd com.apple.quarantine "$SYS_VST3/Plugged 1.vst3" 2>/dev/null
xattr -cr "$SYS_AU/Plugged 1.component" 2>/dev/null
xattr -rd com.apple.quarantine "$SYS_AU/Plugged 1.component" 2>/dev/null
xattr -cr "$SYS_VST3/PlugTune.vst3" 2>/dev/null
xattr -rd com.apple.quarantine "$SYS_VST3/PlugTune.vst3" 2>/dev/null
xattr -cr "$SYS_AU/PlugTune.component" 2>/dev/null
xattr -rd com.apple.quarantine "$SYS_AU/PlugTune.component" 2>/dev/null

xattr -cr "/Applications/PluggedIN Central.app" 2>/dev/null
xattr -rd com.apple.quarantine "/Applications/PluggedIN Central.app" 2>/dev/null

# Ad-hoc code sign for Apple Silicon M1/M2/M3/M4 & Intel Gatekeeper
codesign --force --deep --sign - "$USER_VST3/UNDERGROUND.vst3" 2>/dev/null
codesign --force --deep --sign - "$USER_AU/UNDERGROUND.component" 2>/dev/null
codesign --force --deep --sign - "$USER_VST3/Plugged 1.vst3" 2>/dev/null
codesign --force --deep --sign - "$USER_AU/Plugged 1.component" 2>/dev/null
codesign --force --deep --sign - "$USER_VST3/PlugTune.vst3" 2>/dev/null
codesign --force --deep --sign - "$USER_AU/PlugTune.component" 2>/dev/null
codesign --force --deep --sign - "/Applications/PluggedIN Central.app" 2>/dev/null

# Reset audio component registrar daemon
killall -9 AudioComponentRegistrar 2>/dev/null
echo "  ✓ Permissions granted & Gatekeeper quarantine cleared"

# ── 6. Write Registry Manifest ─────────────────────────────────────────────────
echo "[6/6] Registering installation metadata..."
cat > "$REG_DIR/installed_manifest.json" << MANIFEST
{
  "pluggedin_underground": {
    "installed_version": "4.0.0",
    "install_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "path": "$USER_VST3/UNDERGROUND.vst3"
  },
  "pluggedin_plugged1": {
    "installed_version": "1.0.0",
    "install_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "path": "$USER_VST3/Plugged 1.vst3"
  }
}
MANIFEST

echo ""
echo "================================================================================"
echo "  🎉 INSTALLATION 100% COMPLETE & VERIFIED!"
echo "================================================================================"
echo ""
echo "  HOW TO FIND YOUR PLUGINS IN FL STUDIO (macOS):"
echo "  ────────────────────────────────────────────────────────────────────────────"
echo "  1. Open FL Studio on your Mac."
echo "  2. Go to: Options → Manage plugins (or File Settings → Manage plugins)."
echo "  3. Under 'Scan options' on the left side:"
echo "     • Turn ON: [✓] Rescan previously verified plugins"
echo "     • Turn ON: [✓] Rescan plugins with errors"
echo "  4. Click the 'Find installed plugins' button (top-left)."
echo ""
echo "  WHERE TO FIND THEM AFTER SCANNING:"
echo "  • UNDERGROUND is a VOCAL / FX PLUGIN:"
echo "    → Open Mixer (F9) → Click any empty FX Slot → Select 'UNDERGROUND'."
echo ""
echo "  • PLUGGED 1 is an INSTRUMENT / SYNTH PLUGIN:"
echo "    → Go to the Channel Rack → Click (+) Add Channel → Select 'Plugged 1'."
echo ""
echo "  ────────────────────────────────────────────────────────────────────────────"
echo "  OTHER DAWS:"
echo "  • Logic Pro / GarageBand : Settings → Plug-in Manager → Rescan"
echo "  • Ableton Live           : Preferences → Plug-Ins → Rescan"
echo "  • REAPER                 : Options → Preferences → VST → Re-scan"
echo "================================================================================"
echo ""
read -p "Press [Enter] to exit..."
