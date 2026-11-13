#!/bin/sh
# Packs the build into dist/edigotia-<target>.tar.gz for av: the binary at
# the top with the game's screens (cuss/), motd.txt and art.txt beside it,
# where it reads them from wherever it is started. Worlds and cities are
# saved in the player's own folder ($XDG_STATE_HOME/edigotia, or
# Application Support on macOS), never in the install folder.
#
# Left out: cuss/editor (the interface editor's own screens), notes/,
# screenshots/ and Whales' design spreadsheet.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
stage=stage/edigotia
rm -rf "$stage" && mkdir -p "$stage/cuss" dist
cp out/edigotia LICENSE motd.txt art.txt "$stage/"
cp README.md "$stage/README.upstream.md"
cp asciiverse/README.txt "$stage/README.txt"
cp cuss/*.cuss "$stage/cuss/"
tar -czf "dist/edigotia-$target.tar.gz" -C "$stage" .
ls dist
