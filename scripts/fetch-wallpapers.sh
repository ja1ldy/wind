#!/bin/sh
# Fetch CC0 photo wallpapers from Wikimedia Commons (not generated images).
# Run as your normal user before 'sudo make install'.
set -eu
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
ASSETS="$ROOT/assets"
mkdir -p "$ASSETS"
command -v curl >/dev/null 2>&1 || { echo 'curl is required: sudo apt install curl' >&2; exit 1; }

fetch() {
    name=$1
    url=$2
    dest="$ASSETS/$name.jpg"
    tmp="$dest.download.$$"
    trap 'rm -f "$tmp"' EXIT HUP INT TERM
    printf 'downloading %s from wikimedia commons...\n' "$name"
    # Source files are CC0; resize is provided by Wikimedia's thumbnail server.
    if ! curl --fail --location --silent --show-error --retry 3 \
        --connect-timeout 15 --max-time 240 \
        --user-agent 'windwm-wallpaper-fetch/0.3.1' \
        --output "$tmp" "$url"; then
        echo "failed to fetch $name; see assets/WALLPAPERS.md" >&2
        exit 1
    fi
    magic=$(LC_ALL=C head -c 3 "$tmp" | od -An -tx1 | tr -d ' \n')
    if [ "$magic" != 'ffd8ff' ]; then
        echo "download for $name was not a JPEG image; refusing to install" >&2
        exit 1
    fi
    mv -f "$tmp" "$dest"
    trap - EXIT HUP INT TERM
    printf 'saved %s\n' "$dest"
}

# 1920-pixel-wide photographs: suitable for the Pi's 1920x1080 display.
# The original photo sources, creators and CC0 declarations are in WALLPAPERS.md.
case "${1:-all}" in
    sunflowers|all)
        fetch sunflowers 'https://thumb.wikimedia.org/wikipedia/commons/thumb/3/30/Sunflowers_field_%28Unsplash%29.jpg/1920px-Sunflowers_field_%28Unsplash%29.jpg'
        [ "${1:-all}" = sunflowers ] && exit 0
        ;;
esac
case "${1:-all}" in
    ridge|all)
        fetch ridge 'https://upload.wikimedia.org/wikipedia/commons/thumb/3/31/Mountain_During_Sunset-129099.JPG/1920px-Mountain_During_Sunset-129099.JPG'
        [ "${1:-all}" = ridge ] && exit 0
        ;;
esac
case "${1:-all}" in
    pine|all)
        fetch pine 'https://thumb.wikimedia.org/wikipedia/commons/thumb/5/56/Thick_pine_forest_%28Unsplash%29.jpg/1920px-Thick_pine_forest_%28Unsplash%29.jpg'
        [ "${1:-all}" = pine ] && exit 0
        ;;
esac
case "${1:-all}" in
    dusk|all)
        fetch dusk 'https://thumb.wikimedia.org/wikipedia/commons/thumb/e/e9/Mountain_silhouette_at_sunset_in_Rio_de_Janeiro%2C_Brazil.jpg/1920px-Mountain_silhouette_at_sunset_in_Rio_de_Janeiro%2C_Brazil.jpg'
        [ "${1:-all}" = dusk ] && exit 0
        ;;
    sunflowers|ridge|pine) ;;
    *) echo 'usage: scripts/fetch-wallpapers.sh [all|sunflowers|ridge|pine|dusk]' >&2; exit 2 ;;
esac
printf '\nall requested CC0 wallpapers downloaded. run: sudo make install\n'
