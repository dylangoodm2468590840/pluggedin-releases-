#!/bin/bash
# PluggedIN Audio Systems — macOS 1-Click Universal Installer
# Installs VST3 & AU Audio Units for Logic Pro, Ableton, FL Studio Mac, GarageBand & Reaper
# Supports: macOS 10.13+ | Universal Binary (Apple Silicon + Intel)

echo "========================================================"
echo "  PluggedIN Audio Systems — macOS Installer"
echo "  UNDERGROUND v4.0.0 & PLUGGED 1 v1.0.0"
echo "========================================================"
echo ""

# ── Install Directories ───────────────────────────────────────
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
REG_DIR="$HOME/Library/Application Support/PluggedIN"

mkdir -p "$VST3_DIR"
mkdir -p "$AU_DIR"
mkdir -p "$REG_DIR"

INSTALLED_UNDERGROUND=false
INSTALLED_PLUGGED1=false

# ── UNDERGROUND VST3 ──────────────────────────────────────────
if [ -d "UNDERGROUND.vst3" ]; then
    rm -rf "$VST3_DIR/UNDERGROUND.vst3"
    cp -R "UNDERGROUND.vst3" "$VST3_DIR/"
    echo "[✓] UNDERGROUND VST3 installed → $VST3_DIR/UNDERGROUND.vst3"
    INSTALLED_UNDERGROUND=true
else
    echo "[!] UNDERGROUND.vst3 not found — skipping"
fi

# ── UNDERGROUND AU Component (Logic Pro / GarageBand) ─────────
if [ -d "UNDERGROUND.component" ]; then
    rm -rf "$AU_DIR/UNDERGROUND.component"
    cp -R "UNDERGROUND.component" "$AU_DIR/"
    echo "[✓] UNDERGROUND AU installed  → $AU_DIR/UNDERGROUND.component"
fi

# ── PLUGGED 1 VST3 ───────────────────────────────────────────
if [ -d "Plugged 1.vst3" ]; then
    rm -rf "$VST3_DIR/Plugged 1.vst3"
    cp -R "Plugged 1.vst3" "$VST3_DIR/"
    echo "[✓] PLUGGED 1 VST3 installed  → $VST3_DIR/Plugged 1.vst3"
    INSTALLED_PLUGGED1=true
else
    echo "[!] Plugged 1.vst3 not found — skipping"
fi

# ── PLUGGED 1 AU Component ───────────────────────────────────
if [ -d "Plugged 1.component" ]; then
    rm -rf "$AU_DIR/Plugged 1.component"
    cp -R "Plugged 1.component" "$AU_DIR/"
    echo "[✓] PLUGGED 1 AU installed    → $AU_DIR/Plugged 1.component"
fi

# ── Write Registry Manifest ───────────────────────────────────
cat > "$REG_DIR/installed_manifest.json" << MANIFEST
{
  "pluggedin_underground": {
    "installed_version": "4.0.0",
    "install_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "path": "$VST3_DIR/UNDERGROUND.vst3"
  },
  "pluggedin_plugged1": {
    "installed_version": "1.0.0",
    "install_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "path": "$VST3_DIR/Plugged 1.vst3"
  }
}
MANIFEST

echo ""
echo "========================================================"
echo "  INSTALLATION COMPLETE!"
echo ""
if $INSTALLED_UNDERGROUND; then echo "  ✓ UNDERGROUND v4.0.0"; fi
if $INSTALLED_PLUGGED1;    then echo "  ✓ PLUGGED 1   v1.0.0"; fi
echo ""
echo "  Next steps:"
echo "  • Logic Pro / GarageBand: Preferences → Plug-in Manager → Rescan"
echo "  • Ableton Live: Preferences → Plug-Ins → Rescan"
echo "  • FL Studio: Options → Manage Plugins → Start scan"
echo "  • Reaper: Options → Preferences → VST → Re-scan"
echo "========================================================"
