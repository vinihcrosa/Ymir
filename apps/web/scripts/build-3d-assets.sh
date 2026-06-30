#!/usr/bin/env bash
# Compress the heavy source GLB meshes (Unity/FBX exports) into web-ready,
# meshopt-compressed GLBs under apps/web/public/assets/3d/.
#
# The source packages (hundreds of MB) are NOT committed — point SRC_AREA /
# SRC_VESSEL at the local download folders. Output GLBs are gitignored; run this
# once per machine (or in CI) before using the 3D view.
#
# Usage:
#   SRC_AREA=~/Downloads/baia_de_guanabara \
#   SRC_VESSEL=~/Downloads/celso_furtado_l183_32 \
#   bash apps/web/scripts/build-3d-assets.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEB_DIR="$(dirname "$SCRIPT_DIR")"
OUT_AREA="$WEB_DIR/public/assets/3d/baia_de_guanabara"
OUT_VESSEL="$WEB_DIR/public/assets/3d/vessel"

SRC_AREA="${SRC_AREA:-$HOME/Downloads/baia_de_guanabara}"
SRC_VESSEL="${SRC_VESSEL:-$HOME/Downloads/celso_furtado_l183_32}"

mkdir -p "$OUT_AREA" "$OUT_VESSEL"

# Geometry → meshopt; textures → WebP @ ≤1024px (decodes natively in the browser,
# so no KTX2 transcoder is needed under our COOP/COEP headers). Cuts the heavy
# Unity/FBX exports ~10-20×. Area subset only — terminais_portos.glb (~440MB) is
# intentionally excluded as too heavy for the web view.
pack() {
  echo "[3d] $1 → $2"
  npx --no-install gltf-transform optimize "$1" "$2" \
    --compress meshopt --texture-compress webp --texture-size 1024
}

pack "$SRC_AREA/batimetria_guanabara.glb"  "$OUT_AREA/batimetria.glb"   # bay floor
pack "$SRC_AREA/ilhas.glb"                 "$OUT_AREA/ilhas.glb"        # islands
pack "$SRC_AREA/ponte_rio_niteroi.glb"     "$OUT_AREA/ponte.glb"        # Rio-Niterói bridge
# NOTE: topografia_guanabara.glb is excluded from v1 — 2159 sub-meshes with 2171
# unique per-tile satellite textures don't merge and stay ~115MB even at 512px.
# It needs offline texture-atlasing/retopo before it's web-viable.

# Default vessel mesh (used for every vessel in v1).
pack "$SRC_VESSEL/vessel_web.glb"          "$OUT_VESSEL/celso_furtado.glb"

echo "[3d] done → $WEB_DIR/public/assets/3d"
du -sh "$OUT_AREA" "$OUT_VESSEL"
