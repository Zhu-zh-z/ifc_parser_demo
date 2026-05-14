#!/usr/bin/env bash
set -euo pipefail

BINARY_PATH="${1:-./build/ifc-parse}"

if [[ ! -x "$BINARY_PATH" ]]; then
  echo "Smoke test error: binary not found or not executable: $BINARY_PATH" >&2
  exit 1
fi

run_case() {
  local input_ifc="$1"
  local output_json="$2"
  local expected_extraction_summary="$3"
  local expected_normalization_summary="$4"
  local expected_positions="$5"
  local expected_indices="$6"
  local expected_tx="$7"
  local expected_ty="$8"
  local expected_tz="$9"
  local command_output

  command_output="$("$BINARY_PATH" "$input_ifc" -o "$output_json")"

  if [[ ! -f "$output_json" ]]; then
    echo "Smoke test error: output file missing: $output_json" >&2
    exit 1
  fi

  if [[ "$command_output" != *"$expected_extraction_summary"* ]]; then
    echo "Smoke test error: extraction summary mismatch for $input_ifc" >&2
    echo "Expected to find: $expected_extraction_summary" >&2
    echo "Actual output: $command_output" >&2
    exit 1
  fi

  if [[ "$command_output" != *"$expected_normalization_summary"* ]]; then
    echo "Smoke test error: normalization summary mismatch for $input_ifc" >&2
    echo "Expected to find: $expected_normalization_summary" >&2
    echo "Actual output: $command_output" >&2
    exit 1
  fi

  local json_content
  json_content="$(<"$output_json")"

  if [[ "$json_content" != *"\"geometries\""* ]]; then
    echo "Smoke test error: missing geometries field in $output_json" >&2
    exit 1
  fi
  if [[ "$json_content" != *"\"instances\""* ]]; then
    echo "Smoke test error: missing instances field in $output_json" >&2
    exit 1
  fi
  if [[ "$json_content" != *"\"batchHint\""* ]]; then
    echo "Smoke test error: missing batchHint field in $output_json" >&2
    exit 1
  fi

  local counts
  counts="$(python3 - "$output_json" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = json.load(f)

mesh = data["geometries"][0]["mesh"]
geometry = data["geometries"][0]
instance = data["instances"][0]
print(
    len(mesh["positions"]),
    len(mesh["indices"]),
    len(mesh["normals"]),
    geometry["geometryKey"],
    instance["batchHint"]["geometryKey"],
    instance["batchHint"]["canInstance"],
    len(instance["transform"]),
    instance["transform"][0],
    instance["transform"][5],
    instance["transform"][10],
    instance["transform"][15],
    instance["transform"][12],
    instance["transform"][13],
    instance["transform"][14],
    min(v[0] for v in mesh["positions"]) if mesh["positions"] else 0.0,
    max(v[0] for v in mesh["positions"]) if mesh["positions"] else 0.0,
    min(v[1] for v in mesh["positions"]) if mesh["positions"] else 0.0,
    max(v[1] for v in mesh["positions"]) if mesh["positions"] else 0.0,
    min(v[2] for v in mesh["positions"]) if mesh["positions"] else 0.0,
    max(v[2] for v in mesh["positions"]) if mesh["positions"] else 0.0,
)
PY
)"

  local actual_positions
  local actual_indices
  local actual_normals
  local actual_geometry_key
  local actual_batch_key
  local actual_can_instance
  local transform_len
  local transform_m00
  local transform_m11
  local transform_m22
  local transform_m33
  local transform_tx
  local transform_ty
  local transform_tz
  local min_x
  local max_x
  local min_y
  local max_y
  local min_z
  local max_z
  read -r actual_positions actual_indices actual_normals actual_geometry_key actual_batch_key actual_can_instance transform_len transform_m00 transform_m11 transform_m22 transform_m33 transform_tx transform_ty transform_tz min_x max_x min_y max_y min_z max_z <<<"$counts"

  if [[ "$actual_positions" != "$expected_positions" ]]; then
    echo "Smoke test error: positions count mismatch for $input_ifc" >&2
    echo "Expected positions: $expected_positions" >&2
    echo "Actual positions: $actual_positions" >&2
    exit 1
  fi

  if [[ "$actual_indices" != "$expected_indices" ]]; then
    echo "Smoke test error: indices count mismatch for $input_ifc" >&2
    echo "Expected indices: $expected_indices" >&2
    echo "Actual indices: $actual_indices" >&2
    exit 1
  fi

  if [[ "$actual_normals" != "$actual_positions" ]]; then
    echo "Smoke test error: normals count does not match positions for $input_ifc" >&2
    echo "Positions: $actual_positions" >&2
    echo "Normals: $actual_normals" >&2
    exit 1
  fi

  if [[ "$actual_geometry_key" == "box_3800_1600_2200" ]]; then
    echo "Smoke test error: geometry key is still using the old hardcoded value for $input_ifc" >&2
    exit 1
  fi

  if [[ "$actual_geometry_key" != gk_* ]]; then
    echo "Smoke test error: geometry key format mismatch for $input_ifc" >&2
    echo "Actual geometry key: $actual_geometry_key" >&2
    exit 1
  fi

  if [[ "$actual_batch_key" != "$actual_geometry_key" ]]; then
    echo "Smoke test error: batchHint geometryKey mismatch for $input_ifc" >&2
    echo "Geometry key: $actual_geometry_key" >&2
    echo "Batch key: $actual_batch_key" >&2
    exit 1
  fi

  if [[ "$actual_can_instance" != "True" && "$actual_can_instance" != "true" ]]; then
    echo "Smoke test error: canInstance should be true for supported fixtures: $input_ifc" >&2
    echo "Actual canInstance: $actual_can_instance" >&2
    exit 1
  fi

  if [[ "$transform_len" != "16" ]]; then
    echo "Smoke test error: transform matrix length mismatch for $input_ifc" >&2
    echo "Expected transform length: 16" >&2
    echo "Actual transform length: $transform_len" >&2
    exit 1
  fi

  python3 - "$transform_m00" "$transform_m11" "$transform_m22" "$transform_m33" "$transform_tx" "$transform_ty" "$transform_tz" "$expected_tx" "$expected_ty" "$expected_tz" "$input_ifc" <<'PY'
import sys

m00 = float(sys.argv[1])
m11 = float(sys.argv[2])
m22 = float(sys.argv[3])
m33 = float(sys.argv[4])
tx = float(sys.argv[5])
ty = float(sys.argv[6])
tz = float(sys.argv[7])
expected_tx = float(sys.argv[8])
expected_ty = float(sys.argv[9])
expected_tz = float(sys.argv[10])
input_ifc = sys.argv[11]
tol = 1e-5

if abs(m00 - 1.0) > tol or abs(m11 - 1.0) > tol or abs(m22 - 1.0) > tol or abs(m33 - 1.0) > tol:
    raise SystemExit(
        f"Smoke test error: transform diagonal should be identity for {input_ifc}: "
        f"[{m00}, {m11}, {m22}, {m33}]"
    )

if abs(tx - expected_tx) > tol or abs(ty - expected_ty) > tol or abs(tz - expected_tz) > tol:
    raise SystemExit(
        f"Smoke test error: transform translation mismatch for {input_ifc}: "
        f"expected [{expected_tx}, {expected_ty}, {expected_tz}] got [{tx}, {ty}, {tz}]"
    )
PY

  python3 - "$min_x" "$max_x" "$min_y" "$max_y" "$min_z" "$max_z" "$input_ifc" <<'PY'
import sys

min_x = float(sys.argv[1])
max_x = float(sys.argv[2])
min_y = float(sys.argv[3])
max_y = float(sys.argv[4])
min_z = float(sys.argv[5])
max_z = float(sys.argv[6])
input_ifc = sys.argv[7]
tol = 1e-4

for axis, lower, upper in (("x", min_x, max_x), ("y", min_y, max_y), ("z", min_z, max_z)):
    if abs(lower + upper) > tol:
        raise SystemExit(
            f"Smoke test error: centered bounds mismatch for {input_ifc} on {axis}-axis: "
            f"min={lower} max={upper}"
        )
PY
}

run_case "./tests/box_faceted_brep.ifc" "./tests/output_box_brep.json" \
  "instance=inst_17 geometry=geom_48 source=IfcFacetedBrep faces=6 faceVertices=24" \
  "normalization source=IfcFacetedBrep inputFaces=6 inputFaceVertices=24 cleanedFaces=6 cleanedFaceVertices=24 triangulatedFaces=6 skippedFaces=0 weldedVertices=24 outputIndices=36 degenerateTriangles=0" \
  "24" \
  "36" \
  "1900" \
  "800" \
  "1100"
run_case "./tests/l_shape_faceted_brep.ifc" "./tests/output_l_shape_brep.json" \
  "instance=inst_17 geometry=geom_65 source=IfcFacetedBrep faces=10 faceVertices=48" \
  "normalization source=IfcFacetedBrep inputFaces=10 inputFaceVertices=48 cleanedFaces=10 cleanedFaceVertices=48 triangulatedFaces=10 skippedFaces=0 weldedVertices=48 outputIndices=84 degenerateTriangles=0" \
  "48" \
  "84" \
  "2475" \
  "1900" \
  "1100"
run_case "./tests/box_triangulated_face_set.ifc" "./tests/output_box_triangulated.json" \
  "instance=inst_17 geometry=geom_19 source=IfcTriangulatedFaceSet vertices=8 triangleIndices=36" \
  "normalization source=IfcTriangulatedFaceSet inputVertices=8 inputIndices=36 weldedVertices=24 outputIndices=36 degenerateTriangles=0 invalidIndices=0" \
  "24" \
  "36" \
  "1900" \
  "800" \
  "1100"

echo "Smoke tests passed."
