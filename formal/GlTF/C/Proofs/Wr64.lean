/-
  T1 (write): `tg3__wr_u64le` writes `encodeLE 8 v` at `p + off`.
-/
import GlTF.C.Proofs.Wr32

namespace GlTF.C.Proofs
open GlTF GlTF.C

set_option linter.unusedSimpArgs false
set_option linter.deprecated false

theorem frameLocals0 (a0 a1 a2 : Val) : frameLocals [0, 1, 2] [a0, a1, a2] 0 = some a0 := by
  simp only [frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]; rfl

theorem frameLocals1 (a0 a1 a2 : Val) : frameLocals [0, 1, 2] [a0, a1, a2] 1 = some a1 := by
  simp only [frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]; rfl

theorem frameLocals2 (a0 a1 a2 : Val) : frameLocals [0, 1, 2] [a0, a1, a2] 2 = some a2 := by
  simp only [frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]; rfl

theorem encodeLE_succ (k n : Nat) :
    encodeLE (k + 1) n = (n % 256).toUInt8 :: encodeLE k (n / 256) := rfl

@[simp] theorem and32 (v : Nat) : v &&& 4294967295 = v % 4294967296 := by
  have h := Nat.and_two_pow_sub_one_eq_mod v 32
  simpa using h

/-- The encoding of `k` bytes only depends on the low `8k` bits. -/
theorem encodeLE_mod (k v : Nat) : encodeLE k (v % 256 ^ k) = encodeLE k v := by
  induction k generalizing v with
  | zero => rfl
  | succ k ih =>
    have e1 : (v % 256 ^ (k + 1)) % 256 = v % 256 := by
      rw [Nat.pow_succ, Nat.mul_comm, Nat.mod_mul_right_mod]
    have e2 : (v % 256 ^ (k + 1)) / 256 = (v / 256) % 256 ^ k := by
      rw [Nat.pow_succ, Nat.mul_comm, Nat.mod_mul_right_div_self]
    rw [encodeLE_succ, encodeLE_succ, e1, e2, ih]

/-- Splitting the little-endian encoding at a byte boundary. -/
theorem encodeLE_add (k m v : Nat) :
    encodeLE (k + m) v = encodeLE k v ++ encodeLE m (v / 256 ^ k) := by
  induction k generalizing v with
  | zero => simp [encodeLE]
  | succ k ih =>
    rw [show k + 1 + m = (k + m) + 1 by omega, encodeLE_succ, ih (v / 256), encodeLE_succ]
    congr 1
    rw [Nat.pow_succ, Nat.div_div_eq_div_mul, Nat.mul_comm]
    rfl

/-- `encodeLE 8 v` splits into the low and high 32-bit halves. -/
theorem encodeLE8_eq (v : Nat) :
    encodeLE 8 v = encodeLE 4 (v % 4294967296) ++ encodeLE 4 (v / 4294967296) := by
  have h := encodeLE_add 4 4 v
  rw [show (4 + 4 : Nat) = 8 by omega, show (256 : Nat) ^ 4 = 4294967296 from rfl] at h
  rw [h, encodeLE_mod]

/-- `v & 0xFFFFFFFF` as a `uint32_t`. -/
theorem evLow (loc : Locals) (m : Mem) (ro : Bool) (v : Nat)
    (h2 : loc 2 = some (.int IntTy.u64 v)) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.cast IntTy.u32 (.bin .band (some IntTy.u64) (.var 2)
        (.cast IntTy.u64 (.const (.int IntTy.u32 4294967295)))))
      (.int IntTy.u32 (v % 4294967296)) := by
  refine EvE.of_eq 20 (fun n => ?_)
  simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, evalCast, intOf, IntTy.u64, IntTy.u32,
    IntTy.i32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt, isPtr, C.guard, h2, and255,
    Nat.mod_mod]

/-- `v >> 32` as a `uint32_t`. -/
theorem evHigh (loc : Locals) (m : Mem) (ro : Bool) (v : Nat)
    (h2 : loc 2 = some (.int IntTy.u64 v)) (hvv : v < 18446744073709551616) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.cast IntTy.u32 (.bin .shr (some IntTy.u64) (.var 2) (.const (.int IntTy.i32 32))))
      (.int IntTy.u32 (v / 4294967296)) := by
  refine EvE.of_eq 20 (fun n => ?_)
  simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, evalCast, intOf, IntTy.u64, IntTy.u32,
    IntTy.i32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt, isPtr, C.guard, h2,
    show (2 : Nat) ^ 32 = 4294967296 from rfl, (by omega : v / 4294967296 < 4294967296)]

/-- `off + 4` as a `uint64_t`. -/
theorem evOff4 (loc : Locals) (m : Mem) (ro : Bool) (off : Nat)
    (h1 : loc 1 = some (.int IntTy.u64 off)) (h : off + 4 < 18446744073709551616) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.bin .add (some IntTy.u64) (.var 1) (.cast IntTy.u64 (.const (.int IntTy.u32 4))))
      (.int IntTy.u64 (off + 4)) := by
  refine EvE.of_eq 20 (fun n => ?_)
  simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, evalCast, intOf, IntTy.u64, IntTy.u32,
    IntTy.i32, IntTy.toInt, IntTy.modulus, isPtr, C.guard, h1, h, Nat.mod_eq_of_lt]

end GlTF.C.Proofs

namespace GlTF.C.Proofs

/-- `splice_append` specialised to a 4-byte first write. -/
theorem splice_append4 (blk : Block) (o : Nat) (xs ys : Block) (hx : xs.length = 4)
    (h : o + 4 + ys.length ≤ blk.length) :
    splice (splice blk o xs) (o + 4) ys = splice blk o (xs ++ ys) := by
  have hh : o + xs.length + ys.length ≤ blk.length := by rw [hx]; omega
  rw [show o + 4 = o + xs.length from by rw [hx]]
  exact splice_append blk o xs ys hh

/-- **`tg3__wr_u64le`** writes `encodeLE 8 v` at `p + off`. -/
theorem wr_u64le_writes (loc : Locals) (m : Mem) (b : Nat) (blk : Block) (hm : m b = some blk)
    (off v : Nat) (h0 : loc 0 = some (.ptr b 0)) (h1 : loc 1 = some (.int IntTy.u64 off))
    (h2 : loc 2 = some (.int IntTy.u64 v)) (hlt : off + 8 ≤ blk.length)
    (hbl : blk.length < 18446744073709551616) (hvv : v < 18446744073709551616) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__wr_u64le.body .normal
      ⟨loc, m.upd b (splice blk off (encodeLE 8 v)), false⟩ := by
  have hv4 : off + 4 < 18446744073709551616 := by omega
  have low := evLow loc m false v h2
  have high := evHigh loc m false v h2 hvv
  have off4 := evOff4 loc m false off h1 hv4
  have hargs1 : EvArgs Gen.prog ⟨loc, m, false⟩ [.var 0, .var 1, _] [.ptr b 0, .int IntTy.u64 off,
      .int IntTy.u32 (v % 4294967296)] :=
    EvArgs.cons (EvE.var h0) (EvArgs.cons (EvE.var h1) (EvArgs.cons low EvArgs.nil))
  have hargs2 : EvArgs Gen.prog
      ⟨loc, m.upd b (splice blk off (encodeLE 4 (v % 4294967296))), false⟩
      [.var 0, _, _] [.ptr b 0, .int IntTy.u64 (off + 4), .int IntTy.u32 (v / 4294967296)] :=
    EvArgs.cons (EvE.var h0)
      (EvArgs.cons (evOff4 loc _ false off h1 hv4) (EvArgs.cons (evHigh loc _ false v h2 hvv)
        EvArgs.nil))
  have hm1 : (m.upd b (splice blk off (encodeLE 4 (v % 4294967296)))) b
      = some (splice blk off (encodeLE 4 (v % 4294967296))) := Mem.upd_same _ _ _
  have e1len : (encodeLE 4 (v % 4294967296)).length = 4 := encodeLE_length _ _
  have body1 : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr b 0, .int IntTy.u64 off, .int IntTy.u32 (v % 4294967296)],
        m, false⟩ Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr b 0, .int IntTy.u64 off, .int IntTy.u32 (v % 4294967296)],
        m.upd b (splice blk off (encodeLE 4 (v % 4294967296))), false⟩ :=
    wr_u32le_writes _ m b blk hm off (v % 4294967296)
      (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _) (by omega) hbl
      (Nat.mod_lt _ (by decide))
  have body2 : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr b 0, .int IntTy.u64 (off + 4), .int IntTy.u32 (v / 4294967296)],
        m.upd b (splice blk off (encodeLE 4 (v % 4294967296))), false⟩ Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr b 0, .int IntTy.u64 (off + 4), .int IntTy.u32 (v / 4294967296)],
        m.upd b (splice (splice blk off (encodeLE 4 (v % 4294967296))) (off + 4)
          (encodeLE 4 (v / 4294967296))), false⟩ :=
    by
      simpa [Mem.upd_upd] using
        (wr_u32le_writes _ _ b _ hm1 (off + 4) (v / 4294967296)
          (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
          (by have h := splice_length blk off (encodeLE 4 (v % 4294967296))
                (by rw [encodeLE_length]; omega); omega)
          (by have h := splice_length blk off (encodeLE 4 (v % 4294967296))
                (by rw [encodeLE_length]; omega); omega)
          (by omega : v / 4294967296 < 4294967296))
  have hcat : splice (splice blk off (encodeLE 4 (v % 4294967296))) (off + 4)
      (encodeLE 4 (v / 4294967296))
      = splice blk off (encodeLE 8 v) := by
    rw [splice_append4 blk off _ _ e1len (by simp only [encodeLE_length]; omega), encodeLE8_eq]
  have hstate : (⟨loc, m.upd b (splice (splice blk off (encodeLE 4 (v % 4294967296))) (off + 4)
        (encodeLE 4 (v / 4294967296))), false⟩ : State)
      = ⟨loc, m.upd b (splice blk off (encodeLE 8 v)), false⟩ := by
    rw [hcat]
  simp only [Gen.tg3__wr_u64le]
  have hchain := ExS.seq (ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs1 rfl rfl body1
      (Or.inl rfl)) (ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs2 rfl rfl body2
      (Or.inl rfl))
  simpa [hstate, IntTy.u32, IntTy.u64, IntTy.i32, IntTy.u8] using hchain

end GlTF.C.Proofs
