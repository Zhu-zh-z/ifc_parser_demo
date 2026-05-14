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
  local expected_summary="$3"
  local command_output

  command_output="$("$BINARY_PATH" "$input_ifc" -o "$output_json")"

  if [[ ! -f "$output_json" ]]; then
    echo "Smoke test error: output file missing: $output_json" >&2
    exit 1
  fi

  if [[ "$command_output" != *"$expected_summary"* ]]; then
    echo "Smoke test error: extraction summary mismatch for $input_ifc" >&2
    echo "Expected to find: $expected_summary" >&2
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
}

run_case "./tests/box_faceted_brep.ifc" "./tests/output_box_brep.json" \
  "instance=inst_17 geometry=geom_48 source=IfcFacetedBrep faces=6 faceVertices=24"
run_case "./tests/l_shape_faceted_brep.ifc" "./tests/output_l_shape_brep.json" \
  "instance=inst_17 geometry=geom_65 source=IfcFacetedBrep faces=10 faceVertices=48"
run_case "./tests/box_triangulated_face_set.ifc" "./tests/output_box_triangulated.json" \
  "instance=inst_17 geometry=geom_19 source=IfcTriangulatedFaceSet vertices=8 triangleIndices=36"

echo "Smoke tests passed."
