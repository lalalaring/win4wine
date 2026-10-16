#!/bin/sh
# Run a Windows program with this system's Wine and Proton's translation
# layers (DXVK for Direct3D 9/10/11, VKD3D-Proton for Direct3D 12), in a
# prefix of their own so that the desktop's shell prefix stays untouched:
#   wine-games game.exe [arguments]
# Variables: WINEPREFIX, DXVK_HUD, VKD3D_CONFIG, PROTON_LAYERS
set -e
wine=${WINE:-$HOME/win4wine/.local/wine-install/bin/wine}
prefix=${WINEPREFIX:-$HOME/.local/prefix-games}
layers=${PROTON_LAYERS:-$HOME/.local/proton-layers}
export WINEPREFIX="$prefix"
export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-d3d9,d3d10core,d3d11,dxgi,d3d12,d3d12core=n}"
export DXVK_HUD=${DXVK_HUD:-}
mkdir -p "$prefix"
"$HOME/.local/bin/wine-layers" install "$prefix" >/dev/null 2>&1 || true
exec "$wine" "$@"
