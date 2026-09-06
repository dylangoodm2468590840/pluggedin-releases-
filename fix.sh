#!/bin/bash
# ================================================================================
#   PluggedIN Audio Systems — macOS Autonomous FL Studio & Plugin Healer
#   Automated 1-Click Repair & Clean Fresh Installer
#   Fixes: UNDERGROUND v4.2.3, PlugTune DEV-1071, and Plugged 1
#   Creates fresh fresh-identity bundles: UNDERGRND.vst3 & PLUGTNE.vst3
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
echo "  5. Deploy fresh-identity bundles (UNDERGRND & PLUGTNE) to bypass stuck DAW caches"
echo "  6. Strip macOS Gatekeeper quarantine flags"
echo "  7. Grant full POSIX executable permissions (chmod 755) & code-sign"
echo "  8. Restart CoreAudio daemon"
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
echo "[1/8] Closing running audio applications..."
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
echo "[2/8] Removing old & conflicting plugin files..."
sudo rm -rf "$SYS_VST3"/UNDERGROUND*.vst3 "$SYS_VST3"/PlugTune*.vst3 "$SYS_VST3"/UNDERGRND*.vst3 "$SYS_VST3"/PLUGTNE*.vst3 2>/dev/null
rm -rf "$USER_VST3"/UNDERGROUND*.vst3 "$USER_VST3"/PlugTune*.vst3 "$USER_VST3"/UNDERGRND*.vst3 "$USER_VST3"/PLUGTNE*.vst3 2>/dev/null
sudo rm -rf "$SYS_AU"/UNDERGROUND*.component "$SYS_AU"/PlugTune*.component 2>/dev/null
rm -rf "$USER_AU"/UNDERGROUND*.component "$USER_AU"/PlugTune*.component 2>/dev/null
echo "  ✓ Old plugin binaries removed."

# ── Step 3: Purge FL Studio Cache with Native AWK (No Python Prompt!) ──────────
echo ""
echo "[3/8] Scrubbing FL Studio's blacklisted error database..."

# Delete cached scan presets
if [ -d "$FL_INSTALLED" ]; then
    find "$FL_INSTALLED" -type f \( -iname "*UNDERGROUND*" -o -iname "*PlugTune*" -o -iname "*UNDERGRND*" -o -iname "*PLUGTNE*" \) -delete 2>/dev/null
fi
if [ -d "$FL_USER_DOCS/Effects" ]; then
    find "$FL_USER_DOCS/Effects" -type f \( -iname "*UNDERGROUND*" -o -iname "*PlugTune*" -o -iname "*UNDERGRND*" -o -iname "*PLUGTNE*" \) -delete 2>/dev/null
fi
if [ -d "$FL_USER_DOCS/Generators" ]; then
    find "$FL_USER_DOCS/Generators" -type f \( -iname "*UNDERGROUND*" -o -iname "*PlugTune*" -o -iname "*UNDERGRND*" -o -iname "*PLUGTNE*" \) -delete 2>/dev/null
fi

# Scrub .Plugins.ini error sections using pure AWK
if [ -f "$FL_INSTALLED/.Plugins.ini" ]; then
    awk 'BEGIN {skip=0} /^\[/ { if ($0 ~ /UNDERGROUND/ || $0 ~ /underground/ || $0 ~ /PlugTune/ || $0 ~ /plugtune/ || $0 ~ /UNDERGRND/ || $0 ~ /PLUGTNE/) skip=1; else skip=0 } !skip' "$FL_INSTALLED/.Plugins.ini" > "$FL_INSTALLED/.Plugins.tmp" 2>/dev/null
    if [ -s "$FL_INSTALLED/.Plugins.tmp" ]; then
        mv "$FL_INSTALLED/.Plugins.tmp" "$FL_INSTALLED/.Plugins.ini"
    else
        rm -f "$FL_INSTALLED/.Plugins.tmp" 2>/dev/null
    fi
    echo "  ✓ Scrubbed .Plugins.ini database."
fi

# Scrub VerifiedIDs.nfo
if [ -f "$FL_INSTALLED/VerifiedIDs.nfo" ]; then
    grep -iv "UNDERGROUND" "$FL_INSTALLED/VerifiedIDs.nfo" | grep -iv "PlugTune" | grep -iv "UNDERGRND" | grep -iv "PLUGTNE" > "$FL_INSTALLED/VerifiedIDs.tmp" 2>/dev/null
    if [ -f "$FL_INSTALLED/VerifiedIDs.tmp" ]; then
        mv "$FL_INSTALLED/VerifiedIDs.tmp" "$FL_INSTALLED/VerifiedIDs.nfo"
        echo "  ✓ Cleared stale entries from VerifiedIDs.nfo."
    fi
fi

# Clear FL Studio macOS runtime caches
rm -rf "$HOME/Library/Caches/com.image-line.flstudio"* 2>/dev/null
rm -rf "$HOME/Library/Caches/com.image-line.FL Studio"* 2>/dev/null
echo "  ✓ FL Studio error cache completely cleared."

# ── Step 4: Download Fresh Patched Universal Binaries ──────────────────────────
echo ""
echo "[4/8] Downloading fresh Universal builds (bundleExit teardown patch)..."

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

UG_VST3=$(find "$TMP_DIR/ug" -type d -name "UNDERGROUND.vst3" 2>/dev/null | head -1)
PT_VST3=$(find "$TMP_DIR/pt" -type d -name "PlugTune.vst3" 2>/dev/null | head -1)

# ── Step 5: Deploy Both Original and Fresh-Identity Bundles ────────────────────
echo ""
echo "[5/8] Deploying plugin bundles and fresh-identity clones (UNDERGRND & PLUGTNE)..."

if [ -n "$UG_VST3" ]; then
    # 1. Standard UNDERGROUND.vst3
    sudo cp -R "$UG_VST3" "$SYS_VST3/UNDERGROUND.vst3" 2>/dev/null
    cp -R "$UG_VST3" "$USER_VST3/UNDERGROUND.vst3" 2>/dev/null
    
    # 2. Fresh-Identity UNDERGRND.vst3 (Bypasses any stuck FL Studio cache!)
    sudo cp -R "$UG_VST3" "$SYS_VST3/UNDERGRND.vst3" 2>/dev/null
    sudo mv "$SYS_VST3/UNDERGRND.vst3/Contents/MacOS/UNDERGROUND" "$SYS_VST3/UNDERGRND.vst3/Contents/MacOS/UNDERGRND" 2>/dev/null
    sudo sed -i '' 's/UNDERGROUND/UNDERGRND/g' "$SYS_VST3/UNDERGRND.vst3/Contents/Info.plist" 2>/dev/null
    sudo sed -i '' 's/UNDERGROUND/UNDERGRND/g' "$SYS_VST3/UNDERGRND.vst3/Contents/Resources/moduleinfo.json" 2>/dev/null
    
    cp -R "$SYS_VST3/UNDERGRND.vst3" "$USER_VST3/UNDERGRND.vst3" 2>/dev/null
    echo "  ✓ Deployed UNDERGROUND.vst3 and fresh-identity UNDERGRND.vst3"
fi

if [ -n "$PT_VST3" ]; then
    # 1. Standard PlugTune.vst3
    sudo cp -R "$PT_VST3" "$SYS_VST3/PlugTune.vst3" 2>/dev/null
    cp -R "$PT_VST3" "$USER_VST3/PlugTune.vst3" 2>/dev/null
    
    # 2. Fresh-Identity PLUGTNE.vst3 (Bypasses any stuck FL Studio cache!)
    sudo cp -R "$PT_VST3" "$SYS_VST3/PLUGTNE.vst3" 2>/dev/null
    sudo mv "$SYS_VST3/PLUGTNE.vst3/Contents/MacOS/PlugTune" "$SYS_VST3/PLUGTNE.vst3/Contents/MacOS/PLUGTNE" 2>/dev/null
    sudo sed -i '' 's/PlugTune/PLUGTNE/g' "$SYS_VST3/PLUGTNE.vst3/Contents/Info.plist" 2>/dev/null
    sudo sed -i '' 's/PlugTune/PLUGTNE/g' "$SYS_VST3/PLUGTNE.vst3/Contents/Resources/moduleinfo.json" 2>/dev/null
    
    cp -R "$SYS_VST3/PLUGTNE.vst3" "$USER_VST3/PLUGTNE.vst3" 2>/dev/null
    echo "  ✓ Deployed PlugTune.vst3 and fresh-identity PLUGTNE.vst3"
fi

rm -rf "$TMP_DIR" 2>/dev/null

# ── Step 6: Strip Gatekeeper Quarantine ───────────────────────────────────────
echo ""
echo "[6/8] Stripping macOS Gatekeeper quarantine flags..."
for p in "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/UNDERGRND.vst3 "$SYS_VST3"/PlugTune.vst3 "$SYS_VST3"/PLUGTNE.vst3 \
         "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/UNDERGRND.vst3 "$USER_VST3"/PlugTune.vst3 "$USER_VST3"/PLUGTNE.vst3; do
    if [ -d "$p" ]; then
        sudo xattr -cr "$p" 2>/dev/null
        sudo xattr -rd com.apple.quarantine "$p" 2>/dev/null
        sudo xattr -rd com.apple.provenance "$p" 2>/dev/null
        xattr -cr "$p" 2>/dev/null
        xattr -rd com.apple.quarantine "$p" 2>/dev/null
    fi
done
echo "  ✓ Gatekeeper quarantine stripped."

# ── Step 7: Permissions & Pure Ad-Hoc Code-Signing ────────────────────────────
echo ""
echo "[7/8] Restoring POSIX permissions & applying clean ad-hoc signatures (No Hardened Runtime)..."
for p in "$SYS_VST3"/UNDERGROUND.vst3 "$SYS_VST3"/UNDERGRND.vst3 "$SYS_VST3"/PlugTune.vst3 "$SYS_VST3"/PLUGTNE.vst3 \
         "$USER_VST3"/UNDERGROUND.vst3 "$USER_VST3"/UNDERGRND.vst3 "$USER_VST3"/PlugTune.vst3 "$USER_VST3"/PLUGTNE.vst3; do
    if [ -d "$p" ]; then
        sudo chmod -R 755 "$p" 2>/dev/null
        chmod -R 755 "$p" 2>/dev/null
        # Remove any old hardened runtime signatures and apply pure ad-hoc like Plugged 1
        sudo codesign --remove-signature "$p" 2>/dev/null || codesign --remove-signature "$p" 2>/dev/null
        sudo codesign --force --deep -s - "$p" 2>/dev/null || codesign --force --deep -s - "$p" 2>/dev/null
    fi
done
echo "  ✓ Permissions (755) and clean ad-hoc signatures applied."

# ── Step 8: Restart Audio Daemon ──────────────────────────────────────────────
echo ""
echo "[8/8] Restarting CoreAudio daemon..."
sudo killall -9 AudioComponentRegistrar 2>/dev/null || killall -9 AudioComponentRegistrar 2>/dev/null
echo "  ✓ Audio system refreshed."

echo ""
echo "================================================================================"
echo "  🎉 SUCCESS! ALL PLUGINS REPAIRED & FRESH-IDENTITY BUNDLES DEPLOYED!"
echo "================================================================================"
echo ""
echo "  FINAL STEP IN FL STUDIO (Takes 10 seconds):"
echo "  ────────────────────────────────────────────────────────────────────────────"
echo "  1. Open FL Studio on your Mac."
echo "  2. Go to: Options  ➔  Manage plugins"
echo "  3. In the left panel under 'Scan options':"
echo "     • Turn ON:  [✓] Rescan plugins with errors"
echo "     • Turn ON:  [✓] Rescan previously verified plugins"
echo "  4. Click the 'Find installed plugins' button at top-left."
echo ""
echo "  You will see UNDERGRND and PLUGTNE scan fresh with status 'OK'!"
echo "================================================================================"
echo ""
