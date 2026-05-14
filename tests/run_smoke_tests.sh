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
print(len(mesh["positions"]), len(mesh["indices"]), len(mesh["normals"]))
PY
)"

  local actual_positions
  local actual_indices
  local actual_normals
  read -r actual_positions actual_indices actual_normals <<<"$counts"

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
}

run_case "./tests/box_faceted_brep.ifc" "./tests/output_box_brep.json" \
  "instance=inst_17 geometry=geom_48 source=IfcFacetedBrep faces=6 faceVertices=24" \
  "normalization source=IfcFacetedBrep inputFaces=6 inputFaceVertices=24 cleanedFaces=6 cleanedFaceVertices=24 triangulatedFaces=6 skippedFaces=0 weldedVertices=8 outputIndices=36 degenerateTriangles=0" \
  "8" \
  "36"
run_case "./tests/l_shape_faceted_brep.ifc" "./tests/output_l_shape_brep.json" \
  "instance=inst_17 geometry=geom_65 source=IfcFacetedBrep faces=10 faceVertices=48" \
  "normalization source=IfcFacetedBrep inputFaces=10 inputFaceVertices=48 cleanedFaces=10 cleanedFaceVertices=48 triangulatedFaces=10 skippedFaces=0 weldedVertices=16 outputIndices=84 degenerateTriangles=0" \
  "16" \
  "84"
run_case "./tests/box_triangulated_face_set.ifc" "./tests/output_box_triangulated.json" \
  "instance=inst_17 geometry=geom_19 source=IfcTriangulatedFaceSet vertices=8 triangleIndices=36" \
  "normalization source=IfcTriangulatedFaceSet inputVertices=8 inputIndices=36 weldedVertices=8 outputIndices=36 degenerateTriangles=0 invalidIndices=0" \
  "8" \
  "36"

echo "Smoke tests passed."
