#!/bin/bash
# Double-clicking a Windows program outside the shell.
#
# Inside the win4wine shell a double-click already works: the shell runs the
# program with the session's Wine, which is the one this repository builds and
# which now carries Proton's Direct3D layers (scripts/proton-layers.sh). On the
# Linux side — openKylin's own desktop or file manager — nothing knows what a
# .exe is, so this installs a MIME handler, a desktop entry and a runner that
# use exactly the same Wine and the same layers.
#
# Usage: scripts/exe-handler.sh [--remove]
set -euo pipefail

root=$(cd -- "$(dirname -- "$0")/.." && pwd)
user=${SUDO_USER:-$(id -un)}
home=$(getent passwd "$user" | cut -d: -f6)
runner="$home/.local/bin/win4wine-run-exe"
entry=/usr/share/applications/win4wine-exe.desktop
link=/usr/local/bin/win4wine-run-exe
name=win4wine-exe.desktop
# Every name the shared MIME database has used for a PE image.
mimes=(application/x-ms-dos-executable application/x-dosexec application/x-msdownload
       application/x-msdos-program application/vnd.microsoft.portable-executable
       application/x-wine-extension-exe application/x-wine-extension-com)

# Two files go to system directories, so the script wants root; the MIME
# associations belong to the desktop user and are written as that user.
if [ "$(id -u)" != 0 ]; then
    exec sudo -- "$0" "$@"
fi
as_user() {
    sudo -u "$user" env HOME="$home" "$@"
}

if [ "${1:-}" = --remove ]; then
    rm -f "$entry" "$link" "$runner"
    for mine in "${mimes[@]}"; do
        as_user xdg-mime default '' "$mine" 2>/dev/null || true
    done
    command -v update-desktop-database > /dev/null && update-desktop-database /usr/share/applications || true
    echo 'Removed the handler for Windows programs.'
    exit 0
fi

install -d -m 755 /usr/share/applications
tee "$entry" > /dev/null <<ENTRY
[Desktop Entry]
Type=Application
Name=Windows 程序
Name[en]=Windows program
Comment=Run a Windows program with this system's Wine and Proton's Direct3D layers
Comment[en]=Run a Windows program with this system's Wine and Proton's Direct3D layers
Exec=$link %f
Terminal=false
Icon=wine
MimeType=$(printf '%s;' "${mimes[@]}")
Categories=System;Emulator;
ENTRY
chmod 644 "$entry"

install -d -m 755 -o "$user" "$home/.local/bin"
cat > "$runner" <<'RUNNER'
#!/bin/sh
# Run a Windows program that was clicked outside the shell. It uses the same
# Wine and the same Proton layers as the shell, so a game gets DXVK whether it
# was clicked in the Windows desktop or the Linux one. What it started is
# logged, which is also the only way to see it fail.
#   win4wine-run-exe PROGRAM [ARGUMENTS]
set -e
program=${1:?usage: win4wine-run-exe PROGRAM [ARGUMENTS]}
log=${WIN4WINE_RUN_LOG:-$HOME/.local/state/win4wine/run.log}
mkdir -p "$(dirname "$log")"
shift || true
echo "=== $(date -Is) $program $*" >> "$log"
exec >> "$log" 2>&1
# wine-games is the wrapper that puts the layers in front of Direct3D.
exec "$HOME/.local/bin/wine-games" "$program" "$@"
RUNNER
chmod 755 "$runner"
chown "$user" "$runner"
ln -sf "$runner" "$link"

for mine in "${mimes[@]}"; do
    as_user xdg-mime default "$name" "$mine" 2>/dev/null || true
done
command -v update-desktop-database > /dev/null && update-desktop-database /usr/share/applications || true

echo "Installed: $entry, $link, $runner"
echo 'Double-clicking a .exe on the Linux side now runs it with this Wine and its Proton layers.'
echo "Trouble? Read $home/.local/state/win4wine/run.log"
