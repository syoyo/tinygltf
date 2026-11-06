import Lake
open Lake DSL

package «gltf» where
  leanOptions := #[]

@[default_target]
lean_lib «GlTF» where
  srcDir := "."

/-- Differential-testing driver (see formal/tools/cdiff.py). -/
lean_exe «cdiff» where
  root := `CDiff
