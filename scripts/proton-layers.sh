#!/bin/bash
# Proton's Direct3D layers for this Wine.
#
# Proton itself is a separate Wine build; what a game actually gets from it is
# the translation layers — DXVK for Direct3D 9/10/11 and VKD3D-Proton for
# Direct3D 12 — which are plain Windows DLLs that any Wine can load, as long as
# that Wine was built with Vulkan support. This script takes those DLLs out of
# a Proton build and puts them into a Wine prefix of this installation.
#
# Usage:
#   scripts/proton-layers.sh install [PREFIX] [PROTON-DIR]
#   scripts/proton-layers.sh remove  [PREFIX]
#   scripts/proton-layers.sh check   [PREFIX]
#
# PREFIX defaults to ~/.local/prefix-games and PROTON-DIR to the first Proton
# under ~/.local/proton. A Proton tarball (GE-Proton, or Valve's) can be
# unpacked there: tar xzf GE-Proton*-x86_64.tar.gz -C ~/.local/proton
set -euo pipefail

root=$(cd -- "$(dirname -- "$0")/.." && pwd)
action=${1:-install}
prefix=${2:-${WINEPREFIX:-$HOME/.local/prefix-games}}
proton=${3:-$(find "$HOME/.local/proton" -maxdepth 2 -name files -type d 2>/dev/null | head -1)}
proton=${proton%/files}
wine=${WINE:-$root/.local/wine-install/bin/wine}

windows32="$prefix/drive_c/windows/syswow64"
windows64="$prefix/drive_c/windows/system32"
overrides=(d3d9 d3d10core d3d11 dxgi d3d12 d3d12core)

layers_of() {
    # $1: dxvk or vkd3d-proton, $2: i386-windows or x86_64-windows
    echo "$proton/files/lib/wine/$1/$2"
}

case "$action" in
install)
    [ -d "$prefix/drive_c" ] || { echo "no Wine prefix at $prefix (create it with wineboot first)" >&2; exit 1; }
    [ -d "$proton/files/lib/wine/dxvk" ] || { echo "no Proton layers under $proton" >&2; exit 1; }
    mkdir -p "$windows32"
    # The files in a Proton build are read only, so an earlier copy cannot be
    # overwritten in place: remove it first.
    for source in "$(layers_of dxvk i386-windows)"/*.dll "$(layers_of vkd3d-proton i386-windows)"/*.dll; do
        rm -f "$windows32/$(basename "$source")"
    done
    if [ -d "$windows64" ]; then
        for source in "$(layers_of dxvk x86_64-windows)"/*.dll "$(layers_of vkd3d-proton x86_64-windows)"/*.dll; do
            rm -f "$windows64/$(basename "$source")"
        done
    fi
    cp -a "$(layers_of dxvk i386-windows)"/*.dll "$windows32/"
    cp -a "$(layers_of vkd3d-proton i386-windows)"/*.dll "$windows32/"
    if [ -d "$windows64" ]; then
        cp -a "$(layers_of dxvk x86_64-windows)"/*.dll "$windows64/"
        cp -a "$(layers_of vkd3d-proton x86_64-windows)"/*.dll "$windows64/"
    fi
    for dll in "${overrides[@]}"; do
        WINEPREFIX="$prefix" WINEDEBUG=-all "$wine" reg add 'HKCU\Software\Wine\DllOverrides' \
            /v "$dll" /d native /f > /dev/null 2>&1 || true
    done
    echo "Proton layers installed in $prefix"
    echo "run games with: WINEPREFIX=$prefix DXVK_HUD=1 $wine game.exe"
    ;;
remove)
    for dll in "${overrides[@]}" d3d8; do
        chmod u+w "$windows32/$dll.dll" "$windows64/$dll.dll" 2>/dev/null
        rm -f "$windows32/$dll.dll" "$windows64/$dll.dll" 2>/dev/null
        WINEPREFIX="$prefix" WINEDEBUG=-all "$wine" reg delete 'HKCU\Software\Wine\DllOverrides' \
            /v "$dll" /f > /dev/null 2>&1 || true
    done
    echo "Proton layers removed from $prefix"
    ;;
check)
    # DXVK says which Vulkan device it ended up on in its own log file.
    log=$(ls -t "$prefix"/*/dxvk_d3d*.log "$prefix"/dxvk_d3d*.log 2>/dev/null | head -1 || true)
    if [ -n "$log" ]; then
        echo "DXVK log: $log"
        head -20 "$log"
    else
        echo "no DXVK log in $prefix yet (run a Direct3D program with DXVK_LOG_LEVEL=info)"
    fi
    for dll in "${overrides[@]}"; do
        printf '%-10s %s\n' "$dll" \
            "$(WINEPREFIX="$prefix" WINEDEBUG=-all "$wine" reg query 'HKCU\Software\Wine\DllOverrides' /v "$dll" 2>/dev/null | awk '/REG_SZ/ {print $NF}' || echo '-')"
    done
    ;;
*)
    sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
    ;;
esac
