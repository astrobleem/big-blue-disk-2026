#!/bin/sh
# Cross-build ALFREDO.EXE (episode 2) with OpenWatcom 2 for previews and
# emulator tests. The reference DOS toolchain is Microsoft C 6 with
# src/alfredo2/BUILD.BAT; both compile the same C89 small-model source.
#   WATCOM=/opt/ow tools/alfredo2_build_ow.sh [output.exe]
set -e
: "${WATCOM:?set WATCOM to your OpenWatcom 2 install}"
case "$(uname -s)" in Darwin) bin=$WATCOM/bino64;; *) bin=$WATCOM/binl64;; esac
[ -x "$bin/wcl" ] || bin=$WATCOM/binl
export PATH=$bin:$PATH INCLUDE=$WATCOM/h
root=$(cd "$(dirname "$0")/.." && pwd)
out=${1:-$root/build/ALFREDO.EXE}
mkdir -p "$(dirname "$out")"
tmp=$(mktemp -d)
cp "$root"/src/alfredo2/*.C "$root"/src/alfredo2/*.H "$tmp/"
cp "$root/src/sound/DOSSND.C" "$tmp/"
cp -r "$root/src/sound/win30" "$tmp/"
rm -f "$tmp/HOSTQA.C"
(cd "$tmp" && wcl -q -bt=dos -ms -0 -ox -w4 -fe=ALFREDO.EXE ALFREDO.C DOSSND.C)
cp "$tmp/ALFREDO.EXE" "$out"
python3 "$root/tools/alfredo2_cap.py" "$out"
rm -rf "$tmp"
echo "built $out"
