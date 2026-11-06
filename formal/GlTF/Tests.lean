/-
  Kernel-checked regression tests for the specification. Every `example` is
  decided by the kernel (`decide`), so a wrong verdict fails `lake build`.
  Each case mirrors a C test in tests/tester_v3_c.c.
-/
import GlTF.Validate
import GlTF.Glb
import GlTF.C.Gen

namespace GlTF.Tests
open Validate

def minimal : Model := {}

example : WellFormedStrict minimal := by decide

/-- tester_v3_c: check_strictness_levels, alphaMode "FOO". -/
def badAlpha : Model := { materials := [{ alphaMode := str "FOO" }] }
example : WellFormedSafe badAlpha := by decide
example : ¬ WellFormedStrict badAlpha := by decide
example : validate {} badAlpha = .ok := by decide
example : validate { strictness := .strict } badAlpha = .invalidValue := by decide

/-- camera type "foo". -/
def badCamera : Model := { cameras := [{ type := str "foo" }] }
example : validate { strictness := .strict } badCamera = .invalidValue := by decide
example : Validate.cameraMask { type := str "foo" } = bit 0 := by decide

/-- primitive mode 99. -/
def badMode : Model := { meshes := [{ primitives := [{ mode := 99 }] }] }
example : validate {} badMode = .ok := by decide
example : validate { strictness := .strict } badMode = .invalidValue := by decide

/-- metallicFactor 2.0 is out of [0,1]; NaN fails every range check. -/
example : materialMask { pbr := { metallicFactor := F64.two } } = bit 2 := by decide
example : materialMask { alphaCutoff := F64.nan } = bit 1 := by decide
example : materialMask {} = 0 := by decide

/-- byteStride 5. -/
example : bufferViewMask { buffer := 0, byteLength := 8, byteStride := 5 } = bit 1 := by decide

/-- extensionsRequired must be a subset of extensionsUsed. -/
example : modelMask { extensionsRequired := [str "EXT_x"] } = bit 0 := by decide
example : modelMask { extensionsRequired := [str "EXT_x"], extensionsUsed := [str "EXT_x"] } = 0 := by
  decide

/-- check_oob_index_rejected: out-of-range bufferView is a memory-safety error
    in every strictness mode. -/
def oobIndex : Model :=
  { buffers := [{ byteLength := 4 }],
    bufferViews := [{ buffer := 0, byteLength := 4 }],
    accessors := [{ bufferView := 1000000, componentType := CT.ubyte, count := 1, type := AT.scalar }] }
example : validate {} oobIndex = .invalidIndex := by decide
example : validate { validateIndices := false } oobIndex = .ok := by decide

/-- check_accessor_range_rejected-style case: 2 VEC3 floats need 24 bytes. -/
def accRange (bvLen : UInt64) : Model :=
  { buffers := [{ byteLength := 64 }],
    bufferViews := [{ buffer := 0, byteLength := bvLen }],
    accessors := [{ bufferView := 0, componentType := CT.float, count := 2, type := AT.vec3 }] }
example : validate {} (accRange 24) = .ok := by decide
example : validate {} (accRange 23) = .invalidAccessor := by decide

/-- Material texture indices are validated (added with tg3_validate). -/
example : validate {} { materials := [{ normalTexture := { index := 7 } }] } = .invalidIndex := by
  decide

/-! GLB framing -/

deriving instance DecidableEq for Except

def u32le (v : Nat) : List UInt8 :=
  [(v % 256).toUInt8, (v / 2^8 % 256).toUInt8, (v / 2^16 % 256).toUInt8, (v / 2^24 % 256).toUInt8]

def glbV2 (jsonLen : Nat) (declared : Nat) : List UInt8 :=
  [0x67, 0x6C, 0x54, 0x46] ++ u32le 2 ++ u32le (20 + jsonLen) ++
  u32le declared ++ u32le Glb.chunkJSON ++ List.replicate jsonLen 0x20

example : Glb.parseFrame (glbV2 8 8) =
    .ok { jsonOff := 20, jsonLen := 8, hasJson := true } := by decide +kernel
example : Glb.parseFrame (glbV2 8 12) = .error .chunk := by decide +kernel
example : Glb.parseFrame [0x67, 0x6C, 0x54, 0x46, 3, 0, 0, 0, 0, 0, 0, 0] = .error .header := by
  decide
example : Glb.layout 5 0 false = some ({ jsonOff := 20, jsonLen := 8, hasJson := true }, 28) := by
  decide

end GlTF.Tests

/-! MiniC negative controls: the semantics must flag out-of-bounds access. -/
namespace GlTF.Tests.MiniC
open GlTF.C

def isUB {α} : Res α → Bool
  | .ub => true
  | _ => false

def isOkInt {α} (r : Res (Option Val × α)) (n : Nat) : Bool :=
  match r with
  | .ok (some (.int _ k), _) => k == n
  | _ => false

def mem4 : Mem := fun b => if b = 0 then some [0x67, 0x6C, 0x54, 0x46] else none

/-- In bounds: `tg3__rd_u32le(p, 0)` on "glTF" is 0x46546C67. -/
example : isOkInt (callFunc Gen.prog 50 Gen.fid_tg3__rd_u32le [.ptr 0 0, .int IntTy.u64 0] mem4)
    0x46546C67 = true := by decide +kernel

/-- One byte past the end is undefined behaviour, not a silent read. -/
example : isUB (callFunc Gen.prog 50 Gen.fid_tg3__rd_u32le [.ptr 0 0, .int IntTy.u64 1] mem4)
    = true := by decide +kernel

/-- Writing through the frame pointer inside a read-only (expression) call is
    rejected; storing past a too-small frame object is UB. -/
example : isUB (callFunc Gen.prog 200 Gen.fid_tg3__glb_frame_parse
    [.ptr 0 0, .int IntTy.u64 4, .ptr 1 0]
    (fun b => if b = 0 then some [0x67, 0x6C, 0x54, 0x46] else if b = 1 then some (List.replicate 40 0)
      else none)) = true := by decide +kernel

end GlTF.Tests.MiniC
