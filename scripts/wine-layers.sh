#!/bin/sh
# Put Proton's DXVK and VKD3D-Proton into a Wine prefix (or take them out):
#   wine-layers install [PREFIX]      wine-layers remove [PREFIX]
# The Wine has to support Vulkan for DXVK to load at all.
set -e
action=${1:-install}
prefix=${2:-${WINEPREFIX:-$HOME/.local/prefix-games}}
layers=${PROTON_LAYERS:-$HOME/.local/proton-layers}
wine=${WINE:-$HOME/win4wine/.local/wine-install/bin/wine}
windows32="$prefix/drive_c/windows/syswow64"
windows64="$prefix/drive_c/windows/system32"
case "$action" in
install)
    [ -d "$prefix" ] || { echo "no prefix at $prefix (create it with wineboot first)" >&2; exit 1; }
    mkdir -p "$windows32"
    cp -a "$layers"/dxvk/x32/*.dll "$windows32/" 2>/dev/null || true
    cp -a "$layers"/vkd3d/x32/*.dll "$windows32/" 2>/dev/null || true
    [ -d "$windows64" ] && cp -a "$layers"/dxvk/x64/*.dll "$windows64/" 2>/dev/null || true
    [ -d "$windows64" ] && cp -a "$layers"/vkd3d/x64/*.dll "$windows64/" 2>/dev/null || true
    for dll in d3d9 d3d10core d3d11 dxgi d3d12 d3d12core; do
        WINEPREFIX="$prefix" WINEDEBUG=-all "$wine" reg add 'HKCU\Software\Wine\DllOverrides' \
            /v "$dll" /d native /f > /dev/null 2>&1 || true
    done
    echo "proton layers installed in $prefix"
    ;;
remove)
    for dll in d3d9 d3d10core d3d11 dxgi d3d12 d3d12core d3d8; do
        rm -f "$windows32/$dll.dll" "$windows64/$dll.dll" 2>/dev/null
        WINEPREFIX="$prefix" WINEDEBUG=-all "$wine" reg delete 'HKCU\Software\Wine\DllOverrides' \
            /v "$dll" /f > /dev/null 2>&1 || true
    done
    echo "proton layers removed from $prefix"
    ;;
*)
    echo "usage: wine-layers install|remove [PREFIX]" >&2; exit 2;;
esac
