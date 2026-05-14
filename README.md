# ifc_parser_demo

## Goal

`ifc_parser_demo` is a minimal, dependency-free C++ IFC -> renderer schema conversion demo for a small IFC/STEP geometry subset.

It is not a replacement for mature IFC tools such as IfcOpenShell or xBIM. The demo focuses on `IfcFacetedBrep` / `IfcTriangulatedFaceSet` mesh conversion and rendering hints such as `geometryKey`, `batchHint`, and `canInstance`.

For now, it validates a clear minimal pipeline:

1. Read a `.ifc` file and extract a minimal subset of the STEP Part 21 text format.
2. Pick supported test elements and their `Body` representations, restricted to `IfcFacetedBrep` and `IfcTriangulatedFaceSet`.
3. Normalize both inputs into a unified renderable triangle mesh, then assign a `geometryId` and a content-stable `geometryKey`.
4. Emit JSON in a GPU buffer-oriented layout, plus the minimum hints needed for a future geometry cache and instanced batching.

## Geometry Conversion Notes

The mathematical computation is not the final goal of this demo. It is only the required step for normalizing IFC geometric representations into a renderable mesh.

The current hand-written implementation first covers two geometry branches:

- faceted BRep path: Extract planar polygon faces from `IfcFacetedBrep` / `IfcFace` / `IfcPolyLoop`, handle duplicate points and collinear points, triangulate polygons through 3D -> 2D projection and ear clipping, then orient triangles with a topology pass (edge manifold checks + BFS consistency + signed-volume global direction) instead of shell-center heuristics.
- tessellation path: Read the point list and triangle indices directly from `IfcTriangulatedFaceSet`, without projection or polygon triangulation.
- shared mesh cleanup: Both branches clean duplicate or degenerate input points, filter degenerate triangles, and then preserve or split vertices as needed for hard-edge normal generation before outputting a unified renderable mesh.

The public factory currently lives in `geometry_myimpl.cpp`; the hand-written conversion logic lives under `geometry_normalization/detail/`.

## Field Conventions

- `geometryId`: Per-file geometry reference id.
- `geometryKey`: Stable reuse key generated from mesh content.
- `batchHint`: Rendering metadata, not standard IFC semantics.
- `canInstance`: Whether the geometry is safe for future instanced batching.
- `transform`: Instance matrix serialized as 16 numbers. The translation lives in `transform[12..14]` and currently only stores the local offset that re-centers the canonicalized mesh.
- JSON layout: positions and normals use nested `[x, y, z]` arrays for readability today, with room to switch to flat, GPU-uploadable buffers in a future IFCX-like schema.

## Limitations

This demo intentionally supports only a narrow IFC subset:

- The CLI exports the first supported building element instance only.
- Geometry input is limited to simplified `IfcFacetedBrep` shells and `IfcTriangulatedFaceSet`.
- Geometry reuse is limited to simple planar, hard-edge shapes such as boxes and L-shapes.
- Complex surfaces, holes, booleans, materials, UVs, and unsafe scaling are out of scope.
- Meshes keep real dimensions; positions are centered and the original center is written to `transform`.
- `geometryKey` is stable for emitted mesh content, but not for all possible vertex/index orderings.

## Project Structure

```text
ifc_parser_demo/
├── CMakeLists.txt             # Build the `ifc-parse` demo executable.
├── main.cpp                   # CLI entry point.
├── io/                       # Read `.ifc` file content.
├── ifc_extractor/            # Extract the minimal IFC/STEP subset.
├── geometry_normalization/   # Convert supported geometry into renderable meshes.
│   └── detail/                # Current hand-written geometry implementation.
├── schema_export/            # Generate keys, hints, and JSON schema output.
├── utils/                    # Small math and tolerance helpers.
└── tests/                    # IFC fixtures, generated outputs, and smoke tests.
```

## Build and Test Commands

Build the demo executable:

```bash
cmake -S . -B build
cmake --build build
```

Run the README fixture commands with the built binary:

```bash
./build/ifc-parse ./tests/box_faceted_brep.ifc -o ./tests/output_box_brep.json
./build/ifc-parse ./tests/l_shape_faceted_brep.ifc -o ./tests/output_l_shape_brep.json
./build/ifc-parse ./tests/box_triangulated_face_set.ifc -o ./tests/output_box_triangulated.json
```

Run the smoke test suite:

```bash
ctest --test-dir build
```

Test inputs:

- `box_faceted_brep.ifc`: A box using `IfcFacetedBrep` and six quadrilateral `IfcPolyLoop`s, used for a basic BRep face-to-triangle sanity check.
- `l_shape_faceted_brep.ifc`: An L-shaped concave polygon solid using `IfcFacetedBrep`, used to validate projection, winding, and ear clipping triangulation.
- `box_triangulated_face_set.ifc`: A box using `IfcTriangulatedFaceSet`, used to validate already-triangulated IFC tessellation input.

Simplified example output shape:

```json
{
  "geometries": [
    {
      "geometryId": "geom_box_001",
      "geometryKey": "gk_example_stable_hash",
      "mesh": {
        "positions": [],
        "normals": [],
        "uvs": [],
        "indices": []
      }
    }
  ],
  "instances": [
    {
      "instanceId": "inst_001",
      "geometryId": "geom_box_001",
      "transform": [
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        1900, 800, 1100, 1
      ],
      "batchHint": {
        "geometryKey": "gk_example_stable_hash",
        "canInstance": true
      }
    }
  ]
}
```
