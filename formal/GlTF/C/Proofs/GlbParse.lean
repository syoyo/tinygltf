/-
  T1: `tg3__glb_frame_parse` (translated C) refines `Glb.parseFrame`.

  For every input byte string `d` (|d| < 2^64) the C function, run under the
  MiniC semantics with `data` pointing at `d` and `f` at a 48-byte frame
  object, terminates without undefined behaviour, returns the error code of
  `Glb.parseFrame d`, never writes outside `*f`, and on success leaves in
  `*f` exactly the frame computed by the specification.
-/
import GlTF.C.Gen
import GlTF.C.Hoare
import GlTF.C.Lemmas

set_option linter.unusedSimpArgs false

namespace GlTF.C.Proofs
open GlTF GlTF.C

abbrev u64 := IntTy.u64
abbrev u32 := IntTy.u32
abbrev i32 := IntTy.i32

/-! ## Byte readers -/

theorem rd32_exs (m : Mem) (bd : Nat) (d : List UInt8) (hm : m bd = some d) (o : Nat)
    (ho : o + 4 ≤ d.length) (hl : d.length < 18446744073709551616) (ro : Bool) :
    ExS Gen.prog ⟨frameLocals [0, 1] [.ptr bd 0, .int u64 o], m, ro⟩ Gen.tg3__rd_u32le.body
      (.ret (some (.int u32 (Glb.rd32 d o)))) ⟨frameLocals [0, 1] [.ptr bd 0, .int u64 o], m, ro⟩ := by
  refine ExS.of_eq 30 (fun n => ?_)
  have l1 : o + 1 < 18446744073709551616 := by omega
  have l2 : o + 2 < 18446744073709551616 := by omega
  have l3 : o + 3 < 18446744073709551616 := by omega
  simp [Gen.tg3__rd_u32le, execStmt, evalExpr, frameLocals, evalBin, isPtr, ptrEq, evalBinInt, binInt, evalCast, intOf, load,
    readBytes, hm, ptrAddN, List.idxOf?, List.findIdx?, List.findIdx?.go, IntTy.u64, IntTy.u32,
    IntTy.toInt, Ty.size, IntTy.bytes, IntTy.modulus, l1, l2, l3, Nat.mod_eq_of_lt, C.guard,
    decodeLE_take_one_drop, (by omega : o < d.length), (by omega : o + 1 < d.length),
    (by omega : o + 2 < d.length), (by omega : o + 3 < d.length),
    (by omega : o + 1 + 1 ≤ d.length), (by omega : o + 2 + 1 ≤ d.length),
    (by omega : o + 3 + 1 ≤ d.length), (by omega : o + 1 ≤ d.length),
    (by omega : o + 2 ≤ d.length), (by omega : o + 3 ≤ d.length), (by omega : o ≤ d.length),
    rd32_or]

/-- Calling `tg3__rd_u32le(data, off)` in an expression. -/
theorem rd32_call {s : State} {pe oe : Expr} {bd : Nat} {d : List UInt8} {o : Nat}
    (hm : s.mem bd = some d) (ho : o + 4 ≤ d.length) (hl : d.length < 18446744073709551616)
    (hp : EvE Gen.prog s pe (.ptr bd 0)) (hoe : EvE Gen.prog s oe (.int u64 o)) :
    EvE Gen.prog s (.call 0 [pe, oe]) (.int u32 (Glb.rd32 d o)) :=
  EvE.call (EvArgs.cons hp (EvArgs.cons hoe EvArgs.nil)) rfl rfl
    (rd32_exs s.mem bd d hm o ho hl true)

theorem rd64_exs (m : Mem) (bd : Nat) (d : List UInt8) (hm : m bd = some d) (o : Nat)
    (ho : o + 8 ≤ d.length) (hl : d.length < 18446744073709551616) (ro : Bool) :
    ExS Gen.prog ⟨frameLocals [0, 1] [.ptr bd 0, .int u64 o], m, ro⟩ Gen.tg3__rd_u64le.body
      (.ret (some (.int u64 (Glb.rd64 d o)))) ⟨frameLocals [0, 1] [.ptr bd 0, .int u64 o], m, ro⟩ := by
  refine ExS.of_eq 80 (fun n => ?_)
  simp [Gen.tg3__rd_u64le, Gen.prog, Gen.tg3__rd_u32le, execStmt, evalExpr, evalArgs, frameLocals,
    evalBin, isPtr, ptrEq, evalBinInt, binInt, evalCast, intOf, load, readBytes, hm, ptrAddN, List.idxOf?, List.findIdx?,
    List.findIdx?.go, IntTy.u64, IntTy.u32, IntTy.toInt, Ty.size, IntTy.bytes, IntTy.modulus,
    Nat.mod_eq_of_lt, C.guard, decodeLE_take_one_drop, rd32_or, rd64_or,
    (by omega : o < 18446744073709551616), (by omega : o + 1 < 18446744073709551616),
    (by omega : o + 2 < 18446744073709551616), (by omega : o + 3 < 18446744073709551616),
    (by omega : o + 4 < 18446744073709551616), (by omega : o + 4 + 1 < 18446744073709551616),
    (by omega : o + 4 + 2 < 18446744073709551616), (by omega : o + 4 + 3 < 18446744073709551616),
    (by omega : o < d.length), (by omega : o + 1 < d.length),
    (by omega : o + 2 < d.length), (by omega : o + 3 < d.length),
    (by omega : o + 4 < d.length), (by omega : o + 4 + 1 < d.length),
    (by omega : o + 4 + 2 < d.length), (by omega : o + 4 + 3 < d.length),
    (by omega : o + 1 + 1 ≤ d.length), (by omega : o + 2 + 1 ≤ d.length),
    (by omega : o + 3 + 1 ≤ d.length), (by omega : o + 1 ≤ d.length),
    (by omega : o + 2 ≤ d.length), (by omega : o + 3 ≤ d.length), (by omega : o ≤ d.length),
    (by omega : o + 4 + 1 + 1 ≤ d.length), (by omega : o + 4 + 2 + 1 ≤ d.length),
    (by omega : o + 4 + 3 + 1 ≤ d.length), (by omega : o + 4 + 1 ≤ d.length),
    (by omega : o + 4 + 2 ≤ d.length), (by omega : o + 4 + 3 ≤ d.length), (by omega : o + 4 ≤ d.length),
    rd32_lt, Nat.mod_lt]

/-! ## Frame object -/

def b2n (b : Bool) : Nat := if b then 1 else 0

/-- Bytes of a `tg3__glb_frame` holding `f` (x86-64 layout, 44 bytes of
    fields followed by 4 bytes of tail padding `t`). -/
def frameBytes (f : Glb.Frame) (t : List UInt8) : List UInt8 :=
  encodeLE 8 f.jsonOff ++ (encodeLE 8 f.jsonLen ++ (encodeLE 8 f.binOff ++ (encodeLE 8 f.binLen ++
  (encodeLE 4 (b2n f.hasJson) ++ (encodeLE 4 (b2n f.hasBin) ++ (encodeLE 4 (b2n f.is64) ++ t))))))

/-- The `tg3_error_code` the C function returns for a spec result. -/
def code : Except Glb.Err Glb.Frame → Nat
  | .ok _ => 0
  | .error .header => 22
  | .error .magic => 20
  | .error .version => 21
  | .error .sizeMismatch => 24
  | .error .chunk => 23

syntax "msimp" : tactic
syntax "msimp" "[" Lean.Parser.Tactic.simpLemma,* "]" : tactic
macro_rules
  | `(tactic| msimp [$extra,*]) =>
    `(tactic| simp [execStmt, evalExpr, evalArgs, evalBin, isPtr, ptrEq, evalBinInt, binInt, evalCast,
      evalUn, intOf, load, C.store, readBytes, writeBytes, ptrAddN, ptrAdd, IntTy.u64, IntTy.u32, IntTy.i32,
      IntTy.toInt, IntTy.wrap, Ty.size, IntTy.bytes, IntTy.modulus, Nat.mod_eq_of_lt, C.guard, truthy, boolVal,
      List.idxOf?, List.findIdx?, List.findIdx?.go, List.take_append, List.drop_append, List.drop_drop,
      List.take_of_length_le, List.drop_eq_nil_of_le, List.take_take, decodeLE_take_one_drop,
      decodeLE_encodeLE, frameBytes, b2n, $extra,*, *])
macro_rules
  | `(tactic| msimp) => `(tactic| msimp [Nat.add_zero])

end GlTF.C.Proofs
