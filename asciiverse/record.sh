#!/bin/sh
# Records asciiverse/edigotia.cast, the catalog preview, at 100x30
# (hub/RECORDING.md) with hub/tools/castrec.py from a sibling hub checkout:
# the title screen, a human city founded in a world of five kingdoms, farms
# laid out, weeks passing, the diplomacy screen and a neighbor's terms, the
# military screen, the realm and the world map. The game plays from a throwaway state
# folder; the build in out/ is used (asciiverse/build.sh). Keys are spaced
# out because the game drops keys typed ahead (flushinp()).
#
#   sh asciiverse/record.sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
castrec=${CASTREC:-$root/../hub/tools/castrec.py}
[ -f "$castrec" ] || castrec=$root/../../hub/tools/castrec.py
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
# Then the recording: a human city founded in it, farms laid out, weeks
# passing, a neighbor's terms in Diplomacy, the Military, the Realm and the
# world map.
s="w3;kn;w1.2;ka;w2.5;kenter;w1.5;ky;w1.2;k!;w1.2;kenter;w3"
s="$s;ka;w0.8;k3;w0.8;k1;w0.8;kk;w0.8;kenter;w1.2;kq;w0.8"
s="$s;ka;w0.8;k3;w0.8;k1;w0.8;kl;w0.8;kenter;w1.2;kq;w0.8"
s="$s;k>;w1.5;k>;w1.5;k];w0.6;k];w2.5"
s="$s;k4;w1;k2;w3;kenter;w2.5;ki;w4;kt;w4.5;kq;w1;kq;w1;kq;w1"
s="$s;k4;w1;k3;w3;kr;w2;kb;w1;s15;w1;kenter;w3;kq;w1"
s="$s;k4;w1;k4;w4;kq;w1;k4;w1;k1;w4;kq;w2"
XDG_STATE_HOME=$state python3 "$castrec" "$root/asciiverse/edigotia.cast" "$state" "$s" -- "$inst/edigotia"
