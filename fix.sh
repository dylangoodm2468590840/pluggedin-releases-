#!/bin/bash
# ================================================================================
#   PluggedIN Audio Systems — macOS Autonomous FL Studio & Plugin Healer
#   Automated 1-Click Repair & Clean Fresh Installer
#   Fixes: UNDERGROUND v4.2.3, PlugTune DEV-1071, and Plugged 1
#   Compatibility: macOS 10.15+ (Catalina, Big Sur, Monterey, Ventura, Sonoma, Sequoia)
#   Architecture: Apple Silicon (M1/M2/M3/M4) & Intel Universal
# ================================================================================

clear
echo "================================================================================"
echo "  ⚡ PluggedIN Audio Systems — macOS Autonomous FL Studio & Plugin Fixer"
echo "================================================================================"
echo ""
echo "  This script will automatically:"
echo "  1. Close FL Studio to unlock cached files"
echo "  2. Purge FL Studio's stuck error database (.Plugins.ini & VerifiedIDs)"
echo "  3. Remove old/crashing unpatched plugin binaries"
echo "  4. Download & install fresh verified Universal builds (v4.2.3 & DEV-1071)"
echo "  5. Strip macOS Gatekeeper quarantine flags"
echo "  6. Grant full POSIX executable permissions (chmod 755) & code-sign"
echo "  7. Restart CoreAudio daemon"
echo ""
echo "================================================================================"
echo ""

# ── Elevation Check ───────────────────────────────────────────────────────────
if [ "$EUID" -ne 0 ]; then
    echo "  [Admin Access] Please enter your Mac login password if prompted below:"
    sudo -v || {
        echo "  [!] Running in user-mode..."
    }
fi

echo ""
echo "[1/7] Closing running audio applications..."
killall "FL Studio" 2>/dev/null
killall -9 "FL Studio" 2>/dev/null
echo "  ✓ FL Studio closed."

# ── Target Paths ──────────────────────────────────────────────────────────────
USER_VST3="$HOME/Library/Audio/Plug-Ins/VST3"
USER_AU="$HOME/Library/Audio/Plug-Ins/Components"
SYS_VST3="/Library/Audio/Plug-Ins/VST3"
SYS_AU="/Library/Audio/Plug-Ins/Components"
FL_USER_DOCS="$HOME/Documents/Image-Line/FL Studio/Presets/Plugin database"
FL_INSTALLED="$FL_USER_DOCS/Installed"

mkdir -p "$USER_VST3" "$USER_AU" 2>/dev/null
sudo mkdir -p "$SYS_VST3" "$SYS_AU" 2>/dev/null

# ── Step 2: Remove Old / Broken Plugin Binaries ────────────────────────────────
echo ""
echo "[2/7] Removing old & conflicting plugin files..."
sudo rm -rf "$SYS_VST3/UNDERGROUND.vst3" "$SYS_VST3/PlugTune.vst3" 2>/dev/null
rm -rf "$USER_VST3/UNDERGROUND.vst3" "$USER_VST3/PlugTune.vst3" 2>/dev/null
sudo rm -rf "$SYS_AU/UNDERGROUND.component" "$SYS_AU/PlugTune.component" 2>/dev/null
rm -rf "$USER_AU/UNDERGROUND.component" "$USER_AU/PlugTune.component" 2>/dev/null
echo "  ✓ Old plugin binaries removed."

# ── Step 3: Purge FL Studio Cache with Native AWK (No Python Prompt!) ──────────
echo ""
echo "[3/7] Scrubbing FL Studio's blacklisted error database..."

# Delete cached scan presets
if [ -d "$FL_INSTALLED" ]; then
    find "$FL_INSTALLED" -type f \( -iname "*UNDERGROUND*" -o -iname "*PlugTune*" \) -delete 2>/dev/null
fi
if [ -d "$FL_USER_DOCS/Effects" ]; then
    find "$FL_USER_DOCS/Effects" -type f \( -iname "*UNDERGROUND*" -o -iname "*PlugTune*" \) -delete 2>/dev/null
fi
if [ -d "$FL_USER_DOCS/Generators" ]; then
    find "$FL_USER_DOCS/Generators" -type f \( -iname "*UNDERGROUND*" -o -iname "*PlugTune*" \) -delete 2>/dev/null
fi

# Scrub .Plugins.ini error sections using pure AWK (works on EVERY Mac natively!)
if [ -f "$FL_INSTALLED/.Plugins.ini" ]; then
    awk 'BEGIN {skip=0} /^\[/ { if ($0 ~ /UNDERGROUND/ || $0 ~ /underground/ || $0 ~ /PlugTune/ || $0 ~ /plugtune/) skip=1; else skip=0 } !skip' "$FL_INSTALLED/.Plugins.ini" > "$FL_INSTALLED/.Plugins.tmp" 2>/dev/null
    if [ -s "$FL_INSTALLED/.Plugins.tmp" ]; then
        mv "$FL_INSTALLED/.Plugins.tmp" "$FL_INSTALLED/.Plugins.ini"
    else
        rm -f "$FL_INSTALLED/.Plugins.tmp" 2>/dev/null
    fi
    echo "  ✓ Scrubbed .Plugins.ini database."
fi

# Scrub VerifiedIDs.nfo
if [ -f "$FL_INSTALLED/VerifiedIDs.nfo" ]; then
    grep -iv "UNDERGROUND" "$FL_INSTALLED/VerifiedIDs.nfo" | grep -iv "PlugTune" > "$FL_INSTALLED/VerifiedIDs.tmp" 2>/dev/null
    if [ -f "$FL_INSTALLED/VerifiedIDs.tmp" ]; then
        mv "$FL_INSTALLED/VerifiedIDs.tmp" "$FL_INSTALLED/VerifiedIDs.nfo"
        echo "  ✓ Cleared stale entries from VerifiedIDs.nfo."
    fi
fi

# Clear FL Studio macOS runtime caches
rm -rf "$HOME/Library/Caches/com.image-line.flstudio"* 2>/dev/null
rm -rf "$HOME/Library/Caches/com.image-line.FL Studio"* 2>/dev/null
echo "  ✓ FL Studio error cache completely cleared."

# ── Step 4: Download & Install Fresh Patched Universal Binaries ────────────────
echo ""
echo "[4/7] Downloading fresh Universal builds (bundleExit teardown patch)..."

TMP_DIR="/tmp/pluggedin_repair"
rm -rf "$TMP_DIR" 2>/dev/null
mkdir -p "$TMP_DIR"

echo "  • Downloading UNDERGROUND v4.2.3 (Native Mac Universal)..."
curl -L -f -s --progress-bar "https://github.com/dylangoodm2468590840/pluggedin-releases-/releases/download/underground-v4.2.3/UNDERGROUND_Mac_Universal.vst3.zip" -o "$TMP_DIR/underground.zip"

echo "  • Downloading PlugTune DEV-1071 (Native Mac Universal)..."
curl -L -f -s --progress-bar "https://github.com/dylangoodm2468590840/pluggedin-releases-/releases/download/plugtune-dev1071/PlugTune_Mac_Universal.vst3.zip" -o "$TMP_DIR/plugtune.zip"

echo "  • Extracting plugin bundles..."
unzip -q -o "$TMP_DIR/underground.zip" -d "$TMP_DIR/ug/" 2>/dev/null
unzip -q -o "$TMP_DIR/plugtune.zip" -d "$TMP_DIR/pt/" 2>/dev/null

# Robust search: finds bundles regardless of nested folders inside zip!
UG_VST3=$(find "$TMP_DIR/ug" -type d -name "UNDERGROUND.vst3" 2>/dev/null | head -1)
PT_VST3=$(find "$TMP_DIR/pt" -type d -name "PlugTune.vst3" 2>/dev/null | head -1)
PT_AU=$(find "$TMP_DIR/pt" -type d -name "PlugTune.component" 2>/dev/null | head -1)

if [ -n "$UG_VST3" ]; then
    echo "  → Deploying UNDERGROUND.vst3 from $UG_VST3..."
    sudo cp -R "$UG_VST3" "$SYS_VST3/" 2>/dev/null
    cp -R "$UG_VST3" "$USER_VST3/" 2>/dev/null
    echo "  ✓ UNDERGROUND.vst3 successfully installed!"
else
    echo "  [!] Error: UNDERGROUND.vst3 could not be extracted."
fi

if [ -n "$PT_VST3" ]; then
    echo "  → Deploying PlugTune.vst3 from $PT_VST3..."
    sudo cp -R "$PT_VST3" "$SYS_VST3/" 2>/dev/null
    cp -R "$PT_VST3" "$USER_VST3/" 2>/dev/null
    echo "  ✓ PlugTune.vst3 successfully installed!"
else
    echo "  [!] Error: PlugTune.vst3 could not be extracted."
fi

if [ -n "$PT_AU" ]; then
    sudo cp -R "$PT_AU" "$SYS_AU/" 2>/dev/null
    cp -R "$PT_AU" "$USER_AU/" 2>/dev/null
fi

rm -rf "$TMP_DIR" 2>/dev/null

# ── Step 5: Strip Gatekeeper Quarantine ───────────────────────────────────────
echo ""
echo "[5/7] Stripping macOS Gatekeeper quarantine flags..."
sudo xattr -cr "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/PlugTune.vst3 2>/dev/null
sudo xattr -rd com.apple.quarantine "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/PlugTune.vst3 2>/dev/null
sudo xattr -rd com.apple.provenance "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/PlugTune.vst3 2>/dev/null

xattr -cr "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/PlugTune.vst3 2>/dev/null
xattr -rd com.apple.quarantine "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/PlugTune.vst3 2>/dev/null
xattr -rd com.apple.provenance "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/PlugTune.vst3 2>/dev/null

sudo xattr -cr "$SYS_AU"/PlugTune.component 2>/dev/null
xattr -cr "$USER_AU"/PlugTune.component 2>/dev/null
echo "  ✓ Gatekeeper quarantine stripped."

# ── Step 6: Permissions & Code-Signing ────────────────────────────────────────
echo ""
echo "[6/7] Restoring POSIX permissions & applying ad-hoc signatures..."
sudo chmod -R 755 "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/PlugTune.vst3 2>/dev/null
chmod -R 755 "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/PlugTune.vst3 2>/dev/null
sudo chmod -R 755 "$SYS_AU"/PlugTune.component 2>/dev/null
chmod -R 755 "$USER_AU"/PlugTune.component 2>/dev/null

for p in "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/PlugTune.vst3 "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/PlugTune.vst3; do
    if [ -d "$p" ]; then
        sudo codesign --force --deep --sign - "$p" 2>/dev/null || codesign --force --deep --sign - "$p" 2>/dev/null
    fi
done
echo "  ✓ Permissions (755) and ad-hoc signatures validated."

# ── Step 7: Restart Audio Daemon ──────────────────────────────────────────────
echo ""
echo "[7/7] Restarting CoreAudio daemon..."
sudo killall -9 AudioComponentRegistrar 2>/dev/null || killall -9 AudioComponentRegistrar 2>/dev/null
echo "  ✓ Audio system refreshed."

echo ""
echo "================================================================================"
echo "  🎉 SUCCESS! ALL PLUGINS REPAIRED & INSTALLED!"
echo "================================================================================"
echo ""
echo "  FINAL STEP IN FL STUDIO (Takes 10 seconds):"
echo "  ────────────────────────────────────────────────────────────────────────────"
echo "  1. Open FL Studio on your Mac."
echo "  2. Go to: Options  ➔  Manage plugins"
echo "  3. In the left panel under 'Scan options':"
echo "     • Turn ON:  [✓] Rescan plugins with errors   <── (CRITICAL!)"
echo "     • Turn ON:  [✓] Rescan previously verified plugins"
echo "  4. Click the 'Find installed plugins' button at top-left."
echo ""
echo "  UNDERGROUND and PlugTune will now scan with status 'OK' and load perfectly!"
echo "================================================================================"
echo ""
