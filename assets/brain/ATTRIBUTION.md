# Brain model

- **Project:** [brainproject](https://github.com/itayinbarr/brainproject) (Z-Anatomy / BodyParts3D)
- **License:** CC BY-SA 4.0
- **Files:** `assets/brain`

Education regions map to named cortical meshes in `brain_regions.json`.

## Modifications

The bundled `brain.glb` is a derivative of the brainproject atlas, modified as follows:

1. **Reduced mesh set.** Started from the project's slimmed demo export (cortex,
   cerebellum, and brainstem only; nerves, vessels, and deep structures removed),
   rather than the full atlas.
2. **Draco decompression.** The source used `KHR_draco_mesh_compression`, which
   Qt's Quick 3D `RuntimeLoader` cannot read. The mesh was decompressed to plain
   glTF buffers with [glTF-Transform](https://gltf-transform.dev/)
   (`gltf-transform cp`), which increases file size but makes the model loadable
   at runtime. Geometry, materials, and node names are otherwise unchanged.

Per CC BY-SA 4.0, these changes are indicated here and the model remains under
the same license.