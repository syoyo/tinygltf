/-
  T1 (write), function level: `tg3__wr_u32le` and `tg3__wr_u64le` write exactly
  `encodeLE 4 v` / `encodeLE 8 v` at the requested offset, leaving everything
  else (locals, read-only flag, other blocks) untouched.
-/
import GlTF.C.Proofs.Write

namespace GlTF.C.Proofs
open GlTF GlTF.C

set_option linter.unusedSimpArgs false
set_option linter.deprecated false

open GlTF.C.Proofs

/-- `v &&& 255 = v % 256`. -/
@[simp] theorem and255 (v : Nat) : v &&& 255 = v % 256 := by
  have h := Nat.and_two_pow_sub_one_eq_mod v 8
  simpa using h

@[simp] theorem mod256_mod256 (v : Nat) : v % 256 % 256 = v % 256 := Nat.mod_mod _ _

/-- Evaluation of the byte `(v >> 8k) & 0xFF` cast to `uint8_t`. -/
theorem evByte (loc : Locals) (m : Mem) (ro : Bool) (v kb : Nat)
    (h2 : loc 2 = some (.int IntTy.u32 v)) (hv : v < 4294967296)
    (hkb : kb = 0 ∨ kb = 8 ∨ kb = 16 ∨ kb = 24) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.cast IntTy.u8 (.bin .band (some IntTy.u32)
        (.bin .shr (some IntTy.u32) (.var 2) (.const (.int IntTy.i32 kb)))
        (.const (.int IntTy.u32 255))))
      (.int IntTy.u8 (v / 2 ^ kb % 256)) := by
  rcases hkb with rfl | rfl | rfl | rfl <;>
    refine EvE.of_eq 20 (fun n => ?_) <;>
    simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, evalCast, intOf, IntTy.u32, IntTy.u8,
      IntTy.i32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt, isPtr, C.guard, h2, hv, Nat.mod_mod]

/-- Evaluation of the pointer `p + off + k`. -/
theorem evPtr (loc : Locals) (m : Mem) (ro : Bool) (b : Nat) (blk : Block) (hm : m b = some blk)
    (h0 : loc 0 = some (.ptr b 0)) (h1 : loc 1 = some (.int IntTy.u64 o)) (k : Nat)
    (hk : o + k ≤ blk.length) (hbl : blk.length < 18446744073709551616) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.index (.var 0) (.bin .add (some IntTy.u64) (.var 1)
        (.cast IntTy.u64 (.const (.int IntTy.u32 k)))) 1)
      (.ptr b (o + k)) := by
  refine EvE.of_eq 25 (fun n => ?_)
  simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, evalCast, intOf, load, readBytes, ptrAddN,
    IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Ty.size, Nat.mod_eq_of_lt, C.guard,
    h0, h1, hm, hk, hbl, isPtr, ptrEq, C.guard, Nat.mod_eq_of_lt (by omega : o + k < 18446744073709551616)]

end GlTF.C.Proofs

namespace GlTF.C.Proofs

/-- `simp` set for evaluating one byte store of `tg3__wr_u32le`. -/
syntax "wsimp" ("[" Lean.Parser.Tactic.simpLemma,* "]")? : tactic
macro_rules
  | `(tactic| wsimp [$ts,*]) => `(tactic| simp [Gen.prog, execStmt, evalExpr,
      evalArgs, evalBin, evalBinInt, binInt, evalCast, evalUn, intOf, load, C.store, readBytes,
      writeBytes, ptrAddN, IntTy.u64, IntTy.u32, IntTy.u8, IntTy.i32, IntTy.toInt, IntTy.bytes,
      IntTy.modulus, Ty.size, Nat.mod_eq_of_lt, C.guard, truthy, boolVal, isPtr, ptrEq, $ts,*, *])
  | `(tactic| wsimp) => `(tactic| wsimp [Nat.zero_le])

/-- **`tg3__wr_u32le`** writes `encodeLE 4 v` at `p + off`. -/
theorem wr_u32le_exs (loc : Locals) (m : Mem) (b : Nat) (blk : Block) (hm : m b = some blk)
    (off v : Nat) (h0 : loc 0 = some (.ptr b 0)) (h1 : loc 1 = some (.int IntTy.u64 off))
    (h2 : loc 2 = some (.int IntTy.u32 v)) (hlt : off + 4 ≤ blk.length)
    (hbl : blk.length < 18446744073709551616) (hvv : v < 4294967296) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__wr_u32le.body .normal
      ⟨loc, m.upd b (splice (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
        [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]) (off + 3)
        [v / 16777216 % 256 |>.toUInt8]), false⟩ := by
  have hm1 : (m.upd b (splice blk off [v % 256 |>.toUInt8])) b
      = some (splice blk off [v % 256 |>.toUInt8]) := Mem.upd_same _ _ _
  have hm2 : ((m.upd b (splice blk off [v % 256 |>.toUInt8])).upd b
      (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8])) b
      = some (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8]) :=
    Mem.upd_same _ _ _
  have hm3 : (((m.upd b (splice blk off [v % 256 |>.toUInt8])).upd b
      (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8])).upd b
      (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8])
        (off + 2) [v / 65536 % 256 |>.toUInt8])) b
      = some (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
          [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]) :=
    Mem.upd_same _ _ _
  have st0 : ExS Gen.prog ⟨loc, m, false⟩
      (.store (Ty.int IntTy.u8) (.index (.var 0) (.var 1) 1)
        (.cast IntTy.u8 (.bin .band (some IntTy.u32) (.var 2) (.const (.int IntTy.u32 255)))))
      .normal ⟨loc, m.upd b (splice blk off [v % 256 |>.toUInt8]), false⟩ := by
    refine ExS.of_eq 40 (fun n => ?_)
    wsimp [h0, h1, h2, hlt, hbl, hvv, Nat.mod_lt, (by omega : off ≤ blk.length),
      (by omega : off + 1 ≤ blk.length), (by omega : off < 18446744073709551616),
      (by omega : off + 1 < 18446744073709551616)]
    simp [encodeLE, splice, List.append_assoc, Nat.mod_mod]
  have st1 : ExS Gen.prog ⟨loc, m.upd b (splice blk off [v % 256 |>.toUInt8]), false⟩
      (.store (Ty.int IntTy.u8)
        (.index (.var 0) (.bin .add (some IntTy.u64) (.var 1)
          (.cast IntTy.u64 (.const (.int IntTy.u32 1)))) 1)
        (.cast IntTy.u8 (.bin .band (some IntTy.u32)
          (.bin .shr (some IntTy.u32) (.var 2) (.const (.int IntTy.i32 8)))
          (.const (.int IntTy.u32 255)))))
      .normal ⟨loc, m.upd b (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
        [v / 256 % 256 |>.toUInt8]), false⟩ := by
    refine ExS.of_eq 40 (fun n => ?_)
    wsimp [h0, h1, h2, hlt, hbl, hvv, hm1, Nat.mod_lt,
      (by have h : ([v % 256 |>.toUInt8] : Block).length = 1 := rfl
          have := splice_length blk off [v % 256 |>.toUInt8] (by rw [h]; omega); omega :
        off + 1 ≤ (splice blk off [v % 256 |>.toUInt8]).length),
      (by have h : ([v % 256 |>.toUInt8] : Block).length = 1 := rfl
          have := splice_length blk off [v % 256 |>.toUInt8] (by rw [h]; omega); omega :
        off + 2 ≤ (splice blk off [v % 256 |>.toUInt8]).length),
      (by omega : off + 1 ≤ blk.length),
      (by omega : off + 2 ≤ blk.length), (by omega : off + 1 < 18446744073709551616),
      (by omega : off + 2 < 18446744073709551616)]
    simp [encodeLE, splice, List.append_assoc, Nat.mod_mod, Nat.add_assoc]
  have hm3 : (((m.upd b (splice blk off [v % 256 |>.toUInt8])).upd b
      (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8])).upd b
      (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8])
        (off + 2) [v / 65536 % 256 |>.toUInt8])) b
      = some (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
          [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]) :=
    Mem.upd_same _ _ _
  have hlen1 : (([v % 256 |>.toUInt8] : Block)).length = 1 := rfl
  have hlen2 : (([v / 256 % 256 |>.toUInt8] : Block)).length = 1 := rfl
  have hlen3 : (([v / 65536 % 256 |>.toUInt8] : Block)).length = 1 := rfl
  have hpred : (splice blk off [v % 256 |>.toUInt8]).length = blk.length :=
    splice_length blk off _ (by rw [hlen1]; omega)
  have hcur1 : (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
      [v / 256 % 256 |>.toUInt8]).length = blk.length := by
    rw [splice_length _ _ _ (by rw [hlen2, hpred]; omega)]; exact hpred
  have hcur2 : (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
      [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]).length = blk.length := by
    rw [splice_length _ _ _ (by rw [hlen3, hcur1]; omega)]; exact hcur1
  have st2 : ExS Gen.prog ⟨loc, m.upd b (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
        [v / 256 % 256 |>.toUInt8]), false⟩
      (.store (Ty.int IntTy.u8)
        (.index (.var 0) (.bin .add (some IntTy.u64) (.var 1)
          (.cast IntTy.u64 (.const (.int IntTy.u32 2)))) 1)
        (.cast IntTy.u8 (.bin .band (some IntTy.u32)
          (.bin .shr (some IntTy.u32) (.var 2) (.const (.int IntTy.i32 16)))
          (.const (.int IntTy.u32 255)))))
      .normal ⟨loc, m.upd b (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
        [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]), false⟩ := by
    refine ExS.of_eq 40 (fun n => ?_)
    wsimp [h0, h1, h2, hlt, hbl, hvv, hm2, Nat.mod_lt, hpred, hcur1,
      (by omega : off + 2 ≤ blk.length), (by omega : off + 3 ≤ blk.length),
      (by omega : off + 2 < 18446744073709551616), (by omega : off + 3 < 18446744073709551616)]
    simp [encodeLE, splice, List.append_assoc, Nat.mod_mod, Nat.add_assoc]
  have st3 : ExS Gen.prog ⟨loc, m.upd b (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
        [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]), false⟩
      (.store (Ty.int IntTy.u8)
        (.index (.var 0) (.bin .add (some IntTy.u64) (.var 1)
          (.cast IntTy.u64 (.const (.int IntTy.u32 3)))) 1)
        (.cast IntTy.u8 (.bin .band (some IntTy.u32)
          (.bin .shr (some IntTy.u32) (.var 2) (.const (.int IntTy.i32 24)))
          (.const (.int IntTy.u32 255)))))
      .normal ⟨loc, m.upd b (splice (splice (splice (splice blk off [v % 256 |>.toUInt8]) (off + 1)
        [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]) (off + 3)
        [v / 16777216 % 256 |>.toUInt8]), false⟩ := by
    refine ExS.of_eq 40 (fun n => ?_)
    wsimp [h0, h1, h2, hlt, hbl, hvv, hm3, Nat.mod_lt, hpred, hcur1, hcur2,
      (by omega : off + 3 ≤ blk.length), (by omega : off + 4 ≤ blk.length),
      (by omega : off + 3 < 18446744073709551616), (by omega : off + 4 < 18446744073709551616)]
    simp [encodeLE, splice, List.append_assoc, Nat.mod_mod, Nat.add_assoc]
  have hs1 : splice (splice blk off [v % 256 |>.toUInt8]) (off + 1) [v / 256 % 256 |>.toUInt8]
      = splice blk off ([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8]) :=
    splice_append blk off _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  have hs2 : splice (splice blk off ([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])) (off + 2)
      [v / 65536 % 256 |>.toUInt8]
      = splice blk off (([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])
          ++ [v / 65536 % 256 |>.toUInt8]) :=
    splice_append blk off _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  have hs3 : splice (splice blk off (([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])
        ++ [v / 65536 % 256 |>.toUInt8])) (off + 3) [v / 16777216 % 256 |>.toUInt8]
      = splice blk off ((([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])
          ++ [v / 65536 % 256 |>.toUInt8]) ++ [v / 16777216 % 256 |>.toUInt8]) :=
    splice_append blk off _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  simp only [Gen.tg3__wr_u32le]
  exact ExS.seq st0 (ExS.seq st1 (ExS.seq st2 st3))

/-- Four adjacent one-byte splices of the bytes of `v` equal one 4-byte
    splice of `encodeLE 4 v`. -/
theorem splice4 (blk : Block) (o v : Nat) (h : o + 4 ≤ blk.length) :
    splice (splice (splice (splice blk o [v % 256 |>.toUInt8]) (o + 1)
      [v / 256 % 256 |>.toUInt8]) (o + 2) [v / 65536 % 256 |>.toUInt8]) (o + 3)
      [v / 16777216 % 256 |>.toUInt8] = splice blk o (encodeLE 4 v) := by
  have hs1 : splice (splice blk o [v % 256 |>.toUInt8]) (o + 1) [v / 256 % 256 |>.toUInt8]
      = splice blk o ([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8]) :=
    splice_append blk o _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  have hs2 : splice (splice blk o ([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])) (o + 2)
      [v / 65536 % 256 |>.toUInt8]
      = splice blk o (([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])
          ++ [v / 65536 % 256 |>.toUInt8]) :=
    splice_append blk o _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  have hs3 : splice (splice blk o (([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])
        ++ [v / 65536 % 256 |>.toUInt8])) (o + 3) [v / 16777216 % 256 |>.toUInt8]
      = splice blk o ((([v % 256 |>.toUInt8] ++ [v / 256 % 256 |>.toUInt8])
          ++ [v / 65536 % 256 |>.toUInt8]) ++ [v / 16777216 % 256 |>.toUInt8]) :=
    splice_append blk o _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  rw [hs1, hs2, hs3]
  simp [encodeLE4_eq, Nat.div_one]

/-- **`tg3__wr_u32le`** writes `encodeLE 4 v` at `p + off`. -/
theorem wr_u32le_writes (loc : Locals) (m : Mem) (b : Nat) (blk : Block) (hm : m b = some blk)
    (off v : Nat) (h0 : loc 0 = some (.ptr b 0)) (h1 : loc 1 = some (.int IntTy.u64 off))
    (h2 : loc 2 = some (.int IntTy.u32 v)) (hlt : off + 4 ≤ blk.length)
    (hbl : blk.length < 18446744073709551616) (hvv : v < 4294967296) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__wr_u32le.body .normal
      ⟨loc, m.upd b (splice blk off (encodeLE 4 v)), false⟩ := by
  have hstate : (⟨loc, m.upd b (splice (splice (splice (splice blk off [v % 256 |>.toUInt8])
      (off + 1) [v / 256 % 256 |>.toUInt8]) (off + 2) [v / 65536 % 256 |>.toUInt8]) (off + 3)
      [v / 16777216 % 256 |>.toUInt8]), false⟩ : State)
      = ⟨loc, m.upd b (splice blk off (encodeLE 4 v)), false⟩ := by
    rw [splice4 blk off v hlt]
  simpa [hstate] using wr_u32le_exs loc m b blk hm off v h0 h1 h2 hlt hbl hvv

end GlTF.C.Proofs
