# ifc_parser_demo

## Goal

`ifc_parser_demo` is a minimal IFC -> renderer schema conversion demo implemented in pure C++ without third-party dependencies.

It does not replace mature IFC tools such as IfcOpenShell or xBIM, and it does not use IFCX as an input format. This demo only validates one controlled pipeline: read geometry from a minimal IFC/STEP subset, convert `IfcFacetedBrep` and `IfcTriangulatedFaceSet` into a unified renderable mesh, and export an intermediate schema for the viewer/runtime. The schema includes the metadata needed for geometry reuse and batching.

A mature IFC toolchain can handle full IFC semantic parsing and geometry conversion. This demo focuses on rendering-side hints such as `geometryKey`, `batchHint`, and `canInstance`, helping the viewer reuse GPU buffers and leaving an interface for future instanced batching.

For now, it validates a clear minimal pipeline:

1. Read a `.ifc` file.
2. Extract a minimal subset of the IFC STEP Part 21 text format.
3. Identify one or a few simplified test elements and their `Body` geometric representations.
4. Support two restricted geometry inputs: planar polygon faces extracted from `IfcFacetedBrep`, and already-triangulated `IfcTriangulatedFaceSet`.
5. Convert quadrilateral faces, concave polygons, and already-triangulated faces into a unified renderable triangle mesh.
6. Generate a `geometryId` and a stable `geometryKey` for the converted geometry.
7. Generate JSON for a GPU buffer-oriented layout as a simplified prototype of a future IFCX-like / intermediate schema.
8. Preserve the minimum hint information needed for a future geometry cache and instanced batching.

## Geometry Conversion Notes

The mathematical computation is not the final goal of this demo. It is only the required step for normalizing IFC geometric representations into a renderable mesh.

The current hand-written implementation first covers two geometry branches:

- faceted BRep path: Extract planar polygon faces from `IfcFacetedBrep` / `IfcFace` / `IfcPolyLoop`, handle duplicate points, collinear points, and winding, then generate triangles through 3D -> 2D projection and ear clipping.
- tessellation path: Read the point list and triangle indices directly from `IfcTriangulatedFaceSet`, without projection or polygon triangulation.
- shared mesh cleanup: Both branches perform the required vertex welding, degenerate triangle filtering, and normal generation, then output a unified renderable mesh.

These computations will first live in `geometry_myimpl.cpp` as a minimal implementation.

## Field Conventions

- `geometryId`: Geometry id used for references within the current exported file.
- `geometryKey`: Stable reuse key generated from the geometry content.
- `batchHint`: Rendering metadata generated during conversion; it is not standard IFC semantics.
- `canInstance`: Indicates that the geometry satisfies the geometric requirements for instanced batching. Whether it is actually batched is decided by the viewer/runtime together with material and render-state constraints.

## Project Structure

```text
ifc_parser_demo/
├── io/                       # Read `.ifc` file content.
├── ifc_extractor/            # Extract the minimal IFC/STEP subset.
├── geometry_normalization/   # Convert supported geometry into renderable meshes.
├── schema_export/            # Generate keys, hints, and JSON schema output.
├── utils/                    # Small math and tolerance helpers.
└── tests/                    # IFC fixtures and placeholder outputs.
```

## Test Commands

```bash
ifc-parse ./tests/box_faceted_brep.ifc -o ./tests/output_box_brep.json
ifc-parse ./tests/l_shape_faceted_brep.ifc -o ./tests/output_l_shape_brep.json
ifc-parse ./tests/box_triangulated_face_set.ifc -o ./tests/output_box_triangulated.json
```

Test inputs:

- `box_faceted_brep.ifc`: A box using `IfcFacetedBrep` and six quadrilateral `IfcPolyLoop`s, used for a basic BRep face-to-triangle sanity check.
- `l_shape_faceted_brep.ifc`: An L-shaped concave polygon solid using `IfcFacetedBrep`, used to validate projection, winding, and ear clipping triangulation.
- `box_triangulated_face_set.ifc`: A box using `IfcTriangulatedFaceSet`, used to validate already-triangulated IFC tessellation input.

Example output:

```json
{
  "geometries": [
    {
      "geometryId": "geom_box_001",
      "geometryKey": "box_3800_1600_2200",
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
      "transform": [],
      "batchHint": {
        "geometryKey": "box_3800_1600_2200",
        "canInstance": true
      }
    }
  ]
}
```
