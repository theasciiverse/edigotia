#!/bin/sh
# Records asciiverse/edigotia.cast, the catalog preview, at 100x30
# (hub/RECORDING.md) with hub/tools/castrec.py from a sibling hub checkout:
# the title screen, a human city founded in a world of five kingdoms, a
# farm and a hunting camp laid out, weeks passing, the finance minister and
# the world map. The game plays from a throwaway state
# folder; the build in out/ is used (asciiverse/build.sh). Keys are spaced
# out because the game drops keys typed ahead (flushinp()).
#
#   sh asciiverse/record.sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
castrec=${CASTREC:-$root/../../hub/tools/castrec.py}
inst=$(mktemp -d)
state=$(mktemp -d)
trap 'rm -rf "$inst" "$state"' EXIT
mkdir -p "$inst/cuss"
cp "$root/out/edigotia" "$root/motd.txt" "$root/art.txt" "$inst/"
cp "$root/cuss/"*.cuss "$inst/cuss/"
# First a world, generated off the record (it takes a while): five kingdoms.
g="w2;kg;w1.5;k!;w1;ktab;w0.6"
for r in a b c d e; do g="$g;kA;w0.7;k$r;w0.7"; done
g="$g;w1;kenter;w30;kq;w1"
XDG_STATE_HOME=$state python3 "$castrec" "$state/world.cast" "$state" "$g" -- "$inst/edigotia"
# Then the recording: a human city founded in it, a farm and a hunting camp
# laid out, weeks passing, the finance minister and the world map.
s="w3;kn;w1.2;ka;w2.5;kenter;w1.5;ky;w1.2;k!;w1.2;kenter;w3"
s="$s;ka;w0.8;k3;w0.8;k1;w0.8;kk;w0.8;kenter;w1.5;kq;w0.8"
s="$s;ka;w0.8;k3;w0.8;k2;w0.8;kl;w0.8;kenter;w1.5;kq;w0.8"
s="$s;k>;w1.5;k>;w1.5;k>;w1.5;k>;w2"
s="$s;k2;w1.2;k1;w4;kesc;w1.5;k4;w1.2;k1;w4;kesc;w2"
XDG_STATE_HOME=$state python3 "$castrec" "$root/asciiverse/edigotia.cast" "$state" "$s" -- "$inst/edigotia"
