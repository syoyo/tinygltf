/-
  T2: validation masks and verdicts.
-/
import GlTF.C.Proofs.T3
import GlTF.Validate

namespace GlTF.C.Proofs
open GlTF GlTF.C GlTF.Glb

set_option linter.unusedSimpArgs false
set_option linter.deprecated false
set_option maxRecDepth 100000
set_option maxHeartbeats 10000000

/-! ## Generic struct-field load -/

/-- Load an integer field through a pointer held in local `i`. -/
theorem varLoad (loc : Locals) (m : Mem) (ro : Bool) (i bf : Nat) (fblk : Block)
    (hp : loc i = some (.ptr bf 0)) (hm : m bf = some fblk)
    (o : Nat) (t : IntTy) (w : Nat) (hw : t.width / 8 = w) (ho : o + w ≤ fblk.length)
    (v : Nat) (hv : v < 256 ^ w)
    (he : List.take w (List.drop o fblk) = encodeLE w v) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.load (Ty.int t) (.field (.var i) o)) (.int t v) := by
  refine EvE.of_eq 25 (fun n => ?_)
  simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, load, readBytes, ptrAddN, decodeLE_encodeLE,
    IntTy.toInt, IntTy.bytes, Ty.size, Nat.mod_eq_of_lt, isPtr, ptrEq, C.guard, hp, hm,
    show o ≤ fblk.length by omega, ho, hw, hv, Nat.mod_eq_of_lt hv, he,
    show t.width / 8 = w from hw]

/-- Unsigned 64-bit `<`. -/
theorem evLtU64 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u64 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u64 b)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .lt (some IntTy.u64) aE bE) (boolVal (a < b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, boolVal]

/-- `(uint32_t)x` where `x` is already a `u32`. -/
theorem evCastU32 (loc : Locals) (m : Mem) (ro : Bool) (n : Nat) (e : Expr)
    (hn : n < 4294967296) (h : EvE Gen.prog ⟨loc, m, ro⟩ e (.int IntTy.u32 n)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.u32 e) (.int IntTy.u32 n) := by
  refine EvE.cast h ?_
  simp [evalCast, IntTy.u32, IntTy.modulus, Nat.mod_eq_of_lt hn]

/-- Unsigned 32-bit `|`. -/
theorem evOrU32 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u32 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u32 b)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .bor (some IntTy.u32) aE bE) (.int IntTy.u32 (a ||| b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u32, IntTy.modulus]

/-- Unsigned 32-bit `<<`. -/
theorem evShlU32 (loc : Locals) (m : Mem) (ro : Bool) (a k : Nat) (aE kE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u32 a))
    (hk : EvE Gen.prog ⟨loc, m, ro⟩ kE (.int IntTy.i32 k))
    (hk31 : k < 2147483648) (hk32 : k < 32) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .shl (some IntTy.u32) aE kE)
      (.int IntTy.u32 ((a * 2 ^ k) % 4294967296)) := by
  refine EvE.bin ha hk ?_
  have hki : IntTy.toInt IntTy.i32 k = (k : Int) := by
    simp [IntTy.toInt, IntTy.i32, IntTy.modulus]; omega
  have hk31' : ¬ 2147483648 ≤ k := by omega
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u32, IntTy.i32, IntTy.modulus,
    IntTy.toInt, IntTy.width, hki, hk31', hk32, show (k : Int) < 32 from by omega, guard]

/-! ## `tg3__spec_buffer_mask` -/

theorem bufferMask_toNat (b : Buffer) :
    (Validate.bufferMask b).toNat = if b.byteLength < 1 then 1 else 0 := by
  unfold Validate.bufferMask Validate.flag Validate.bit
  by_cases h : b.byteLength < 1 <;> simp [h]



/-! ## `tg3__spec_buffer_mask` -/

theorem buffer_mask_correct (loc : Locals) (m : Mem) (bp : Nat) (blk : Block)
    (b : Buffer)
    (h0 : loc 0 = some (.ptr bp 0)) (hm : m bp = some blk)
    (hblk : 24 ≤ blk.length)
    (hlen : List.take 8 (List.drop 16 blk) = encodeLE 8 b.byteLength.toNat) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__spec_buffer_mask.body
      (.ret (some (.int IntTy.u32 (Validate.bufferMask b).toNat)))
      ⟨loc.upd 1 (.int IntTy.u32 (Validate.bufferMask b).toNat), m, false⟩ := by
  have hload : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 0) 16)) (.int IntTy.u64 b.byteLength.toNat) :=
    varLoad (loc.upd 1 (.int IntTy.u32 0)) m false 0 bp blk (by simp [h0]) hm 16 IntTy.u64 8
      (by rfl) (by omega) b.byteLength.toNat (by simpa using UInt64.toNat_lt b.byteLength) hlen
  have hlt : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
      (.bin .lt (some IntTy.u64)
        (.load (Ty.int IntTy.u64) (.field (.var 0) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 1))))
      (boolVal (b.byteLength.toNat < 1)) :=
    evLtU64 (loc.upd 1 (.int IntTy.u32 0)) m false _ 1 _ _ hload
      (evCastU64U32 (loc.upd 1 (.int IntTy.u32 0)) m false 1 (by decide))
  have hset0 : ExS Gen.prog ⟨loc, m, false⟩
      (.set 1 (.cast IntTy.u32 (.const (.int IntTy.i32 0)))) .normal
      ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩ :=
    ExS.set (EvE.cast (EvE.const _) (by simp [evalCast, IntTy.u32, IntTy.i32, IntTy.toInt, IntTy.wrap, IntTy.modulus]))
  by_cases hc : b.byteLength < 1
  · have hc' : b.byteLength.toNat < 1 := UInt64.lt_iff_toNat_lt.mp hc
    have hspec : (Validate.bufferMask b).toNat = 1 := by
      rw [bufferMask_toNat]; simp [hc]
    rw [hspec]
    -- then-branch
    have hvar : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩ (.var 1)
        (.int IntTy.u32 0) := EvE.var (by simp)
    have hcast1 : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.cast IntTy.u32 (.var 1)) (.int IntTy.u32 0) :=
      evCastU32 (loc.upd 1 (.int IntTy.u32 0)) m false 0 (.var 1) (by decide) hvar
    have hshl : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0)))
        (.int IntTy.u32 1) := by
      have h := evShlU32 (loc.upd 1 (.int IntTy.u32 0)) m false 1 0
        (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0))
        (EvE.const _) (EvE.const _) (by decide) (by decide)
      simpa using h
    have hcast2 : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.cast IntTy.u32
          (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0))))
        (.int IntTy.u32 1) := by
      refine EvE.cast hshl ?_
      simp [evalCast, IntTy.u32, IntTy.modulus, Nat.mod_eq_of_lt (by decide : 1 < 4294967296)]
    have hor : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.bin .bor (some IntTy.u32) (.cast IntTy.u32 (.var 1))
          (.cast IntTy.u32
            (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0)))))
        (.int IntTy.u32 1) := by
      have h := evOrU32 (loc.upd 1 (.int IntTy.u32 0)) m false 0 1
        (.cast IntTy.u32 (.var 1))
        (.cast IntTy.u32
          (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0))))
        hcast1 hcast2
      simpa [show (0 ||| 1) = 1 from by decide] using h
    have hset1 : ExS Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.set 1 (.cast IntTy.u32
          (.bin .bor (some IntTy.u32) (.cast IntTy.u32 (.var 1))
            (.cast IntTy.u32
              (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0)))))))
        .normal ⟨(loc.upd 1 (.int IntTy.u32 0)).upd 1 (.int IntTy.u32 1), m, false⟩ := by
      have hcastOut : EvE Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
          (.cast IntTy.u32
            (.bin .bor (some IntTy.u32) (.cast IntTy.u32 (.var 1))
              (.cast IntTy.u32
                (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0))))))
          (.int IntTy.u32 1) :=
        evCastU32 (loc.upd 1 (.int IntTy.u32 0)) m false 1 _ (by decide) hor
      exact ExS.set hcastOut
    have hite : ExS Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.ite (.bin .lt (some IntTy.u64)
            (.load (Ty.int IntTy.u64) (.field (.var 0) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 1))))
          (.set 1 (.cast IntTy.u32
            (.bin .bor (some IntTy.u32) (.cast IntTy.u32 (.var 1))
              (.cast IntTy.u32
                (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0)))))))
          .skip) .normal
        ⟨(loc.upd 1 (.int IntTy.u32 0)).upd 1 (.int IntTy.u32 1), m, false⟩ :=
      ExS.ite (bc := true) hlt (by simp [truthy, boolVal, hc']) hset1
    have hret : ExS Gen.prog ⟨(loc.upd 1 (.int IntTy.u32 0)).upd 1 (.int IntTy.u32 1), m, false⟩
        (.ret (some (.var 1))) (.ret (some (.int IntTy.u32 1)))
        ⟨(loc.upd 1 (.int IntTy.u32 0)).upd 1 (.int IntTy.u32 1), m, false⟩ :=
      ExS.ret (EvE.var (by simp))
    have hbody := ExS.seq hset0 (ExS.seq hite hret)
    simpa [Gen.tg3__spec_buffer_mask, Locals.upd_upd, IntTy.u32, IntTy.i32, IntTy.u64] using hbody
  · have hc' : ¬ b.byteLength.toNat < 1 := by
      intro h; exact hc (UInt64.lt_iff_toNat_lt.mpr h)
    have hspec : (Validate.bufferMask b).toNat = 0 := by
      rw [bufferMask_toNat]; simp [hc]
    rw [hspec]
    have hite : ExS Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.ite (.bin .lt (some IntTy.u64)
            (.load (Ty.int IntTy.u64) (.field (.var 0) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 1))))
          (.set 1 (.cast IntTy.u32
            (.bin .bor (some IntTy.u32) (.cast IntTy.u32 (.var 1))
              (.cast IntTy.u32
                (.bin .shl (some IntTy.u32) (.const (.int IntTy.u32 1)) (.const (.int IntTy.i32 0)))))))
          .skip) .normal ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩ :=
      ExS.ite (bc := false) hlt (by simp [truthy, boolVal, hc']) ExS.skip
    have hret : ExS Gen.prog ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩
        (.ret (some (.var 1))) (.ret (some (.int IntTy.u32 0)))
        ⟨loc.upd 1 (.int IntTy.u32 0), m, false⟩ :=
      ExS.ret (EvE.var (by simp))
    have hbody := ExS.seq hset0 (ExS.seq hite hret)
    simpa [Gen.tg3__spec_buffer_mask, IntTy.u32, IntTy.i32, IntTy.u64] using hbody

end GlTF.C.Proofs
