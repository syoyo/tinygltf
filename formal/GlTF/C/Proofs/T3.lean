/-
  T3 helper refinements: component/num components, base64 helpers.
-/
import GlTF.C.Proofs.Layout
import GlTF.Model
import GlTF.B64

namespace GlTF.C.Proofs
open GlTF GlTF.C GlTF.Glb

set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false
set_option linter.deprecated false
set_option maxRecDepth 100000

theorem evalBin_eq_i32 (a c : Nat) (ha : a < 4294967296) (hc : c < 2147483648) :
    evalBin .eq (some IntTy.i32) (.int IntTy.i32 a) (.int IntTy.i32 c)
      = .ok (boolVal (a = c)) := by
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.i32, IntTy.toInt, IntTy.modulus,
    Nat.mod_eq_of_lt ha, boolVal]
  by_cases ha31 : 2147483648 ≤ a
  · have hc31 : ¬ 2147483648 ≤ c := by omega
    have hne : a ≠ c := by omega
    have hne' : (a : Int) - 4294967296 ≠ (c : Int) := by omega
    simp [ha31, hc31, hne, hne']
  · have hc31 : ¬ 2147483648 ≤ c := by omega
    by_cases h : a = c
    · subst h; simp
    · have h' : (a : Int) ≠ (c : Int) := by omega
      simp [ha31, hc31, h, h']

theorem evMulU64 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u64 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u64 b))
    (h : a * b < 18446744073709551616) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .mul (some IntTy.u64) aE bE) (.int IntTy.u64 (a * b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, Nat.mod_eq_of_lt h]

theorem evDivU64 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u64 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u64 b))
    (hb0 : b ≠ 0) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .div (some IntTy.u64) aE bE) (.int IntTy.u64 (a / b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, hb0]

theorem b64_decoded_len_correct (loc : Locals) (m : Mem) (n : Nat)
    (h0 : loc 0 = some (.int IntTy.u64 n))
    (hn3 : n * 3 < 18446744073709551616) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__b64_decoded_len.body
      (.ret (some (.int IntTy.u64 (GlTF.B64.decodedLen n)))) ⟨loc, m, false⟩ := by
  have hmul : EvE Gen.prog ⟨loc, m, false⟩
      (.bin .mul (some IntTy.u64) (.var 0) (Expr.cast IntTy.u64 (Expr.const (Val.int IntTy.u32 3))))
      (.int IntTy.u64 (n * 3)) :=
    evMulU64 loc m false n 3 (.var 0)
      (Expr.cast IntTy.u64 (Expr.const (Val.int IntTy.u32 3))) (EvE.var h0)
      (evCastU64U32 loc m false 3 (by decide)) hn3
  have hdiv : EvE Gen.prog ⟨loc, m, false⟩
      (.bin .div (some IntTy.u64)
        (.bin .mul (some IntTy.u64) (.var 0) (Expr.cast IntTy.u64 (Expr.const (Val.int IntTy.u32 3))))
        (Expr.cast IntTy.u64 (Expr.const (Val.int IntTy.u32 4))))
      (.int IntTy.u64 (n * 3 / 4)) :=
    evDivU64 loc m false (n * 3) 4
      (.bin .mul (some IntTy.u64) (.var 0) (Expr.cast IntTy.u64 (Expr.const (Val.int IntTy.u32 3))))
      (Expr.cast IntTy.u64 (Expr.const (Val.int IntTy.u32 4))) hmul
      (evCastU64U32 loc m false 4 (by decide)) (by decide)
  simp only [Gen.tg3__b64_decoded_len, GlTF.B64.decodedLen]
  exact ExS.ret hdiv

theorem int32_ofNat_ne (n c : Nat) (hn : n < 4294967296) (hc : c < 2147483648)
    (h : n ≠ c) : (Int32.ofNat n) ≠ (Int32.ofNat c) := by
  intro he
  have hb := congrArg (fun x : Int32 => x.toBitVec.toNat) he
  simp [Int32.ofNat, Int32.toBitVec, BitVec.toNat_ofNat,
    Nat.mod_eq_of_lt hn, Nat.mod_eq_of_lt (by omega : c < 4294967296)] at hb
  exact h hb


theorem evNegOneI32 (loc : Locals) (m : Mem) (ro : Bool) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.un .neg IntTy.i32 (.const (.int IntTy.i32 1)))
      (.int IntTy.i32 4294967295) := by
  refine EvE.un (EvE.const _) ?_
  rfl

theorem component_size_correct (loc : Locals) (m : Mem) (n : Nat)
    (h0 : loc 0 = some (.int IntTy.i32 n)) (hn : n < 4294967296) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3_component_size.body
      (.ret (some (.int IntTy.i32 (GlTF.wrap32 (GlTF.componentSize (Int32.ofNat n))))))
      ⟨loc, m, false⟩ := by
  have evEq (c : Nat) (hc : c < 2147483648) :
      EvE Gen.prog ⟨loc, m, false⟩
        (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 c)))
        (boolVal (n = c)) :=
    EvE.bin (EvE.var h0) (EvE.const _) (evalBin_eq_i32 n c hn hc)
  have spec1 (h : n = 5120 ∨ n = 5121) :
      GlTF.wrap32 (GlTF.componentSize (Int32.ofNat n)) = 1 := by
    rcases h with rfl | rfl <;> decide
  have spec2 (h : n = 5122 ∨ n = 5123) :
      GlTF.wrap32 (GlTF.componentSize (Int32.ofNat n)) = 2 := by
    rcases h with rfl | rfl <;> decide
  have spec4 (h : n = 5124 ∨ n = 5125 ∨ n = 5126) :
      GlTF.wrap32 (GlTF.componentSize (Int32.ofNat n)) = 4 := by
    rcases h with rfl | rfl | rfl <;> decide
  have spec8 (h : n = 5130) : GlTF.wrap32 (GlTF.componentSize (Int32.ofNat n)) = 8 := by
    subst h; decide
  have specM (h1 : n ≠ 5120) (h2 : n ≠ 5121) (h3 : n ≠ 5122) (h4 : n ≠ 5123)
      (h5 : n ≠ 5124) (h6 : n ≠ 5125) (h7 : n ≠ 5126) (h8 : n ≠ 5130) :
      GlTF.wrap32 (GlTF.componentSize (Int32.ofNat n)) = 4294967295 := by
    have e1 : Int32.ofNat n ≠ (5120 : Int32) := by simpa using int32_ofNat_ne n 5120 hn (by decide) h1
    have e2 : Int32.ofNat n ≠ (5121 : Int32) := by simpa using int32_ofNat_ne n 5121 hn (by decide) h2
    have e3 : Int32.ofNat n ≠ (5122 : Int32) := by simpa using int32_ofNat_ne n 5122 hn (by decide) h3
    have e4 : Int32.ofNat n ≠ (5123 : Int32) := by simpa using int32_ofNat_ne n 5123 hn (by decide) h4
    have e5 : Int32.ofNat n ≠ (5124 : Int32) := by simpa using int32_ofNat_ne n 5124 hn (by decide) h5
    have e6 : Int32.ofNat n ≠ (5125 : Int32) := by simpa using int32_ofNat_ne n 5125 hn (by decide) h6
    have e7 : Int32.ofNat n ≠ (5126 : Int32) := by simpa using int32_ofNat_ne n 5126 hn (by decide) h7
    have e8 : Int32.ofNat n ≠ (5130 : Int32) := by simpa using int32_ofNat_ne n 5130 hn (by decide) h8
    simp [GlTF.componentSize, GlTF.CT.byte, GlTF.CT.ubyte, GlTF.CT.short, GlTF.CT.ushort,
      GlTF.CT.int, GlTF.CT.uint, GlTF.CT.float, GlTF.CT.double, e1, e2, e3, e4, e5, e6, e7, e8,
      GlTF.wrap32]
  simp only [Gen.tg3_component_size]
  by_cases h1 : n = 5120
  · have hc : EvE Gen.prog ⟨loc, m, false⟩
        (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5120)))
        (boolVal true) := by simpa [h1] using evEq 5120 (by decide)
    rw [spec1 (Or.inl h1)]
    exact ExS.ite (bc := true) hc (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
  · have hc : EvE Gen.prog ⟨loc, m, false⟩
        (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5120)))
        (boolVal false) := by simpa [h1] using evEq 5120 (by decide)
    apply ExS.ite (bc := false) hc (by simp [truthy, boolVal])
    by_cases h2 : n = 5121
    · have hc2 : EvE Gen.prog ⟨loc, m, false⟩
          (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5121)))
          (boolVal true) := by simpa [h2] using evEq 5121 (by decide)
      rw [spec1 (Or.inr h2)]
      exact ExS.ite (bc := true) hc2 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
    · have hc2 : EvE Gen.prog ⟨loc, m, false⟩
          (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5121)))
          (boolVal false) := by simpa [h2] using evEq 5121 (by decide)
      apply ExS.ite (bc := false) hc2 (by simp [truthy, boolVal])
      by_cases h3 : n = 5122
      · have hc3 : EvE Gen.prog ⟨loc, m, false⟩
            (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5122)))
            (boolVal true) := by simpa [h3] using evEq 5122 (by decide)
        rw [spec2 (Or.inl h3)]
        exact ExS.ite (bc := true) hc3 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
      · have hc3 : EvE Gen.prog ⟨loc, m, false⟩
            (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5122)))
            (boolVal false) := by simpa [h3] using evEq 5122 (by decide)
        apply ExS.ite (bc := false) hc3 (by simp [truthy, boolVal])
        by_cases h4 : n = 5123
        · have hc4 : EvE Gen.prog ⟨loc, m, false⟩
              (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5123)))
              (boolVal true) := by simpa [h4] using evEq 5123 (by decide)
          rw [spec2 (Or.inr h4)]
          exact ExS.ite (bc := true) hc4 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
        · have hc4 : EvE Gen.prog ⟨loc, m, false⟩
              (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5123)))
              (boolVal false) := by simpa [h4] using evEq 5123 (by decide)
          apply ExS.ite (bc := false) hc4 (by simp [truthy, boolVal])
          by_cases h5 : n = 5124
          · have hc5 : EvE Gen.prog ⟨loc, m, false⟩
                (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5124)))
                (boolVal true) := by simpa [h5] using evEq 5124 (by decide)
            rw [spec4 (Or.inl h5)]
            exact ExS.ite (bc := true) hc5 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
          · have hc5 : EvE Gen.prog ⟨loc, m, false⟩
                (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5124)))
                (boolVal false) := by simpa [h5] using evEq 5124 (by decide)
            apply ExS.ite (bc := false) hc5 (by simp [truthy, boolVal])
            by_cases h6 : n = 5125
            · have hc6 : EvE Gen.prog ⟨loc, m, false⟩
                  (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5125)))
                  (boolVal true) := by simpa [h6] using evEq 5125 (by decide)
              rw [spec4 (Or.inr (Or.inl h6))]
              exact ExS.ite (bc := true) hc6 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
            · have hc6 : EvE Gen.prog ⟨loc, m, false⟩
                  (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5125)))
                  (boolVal false) := by simpa [h6] using evEq 5125 (by decide)
              apply ExS.ite (bc := false) hc6 (by simp [truthy, boolVal])
              by_cases h7 : n = 5126
              · have hc7 : EvE Gen.prog ⟨loc, m, false⟩
                    (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5126)))
                    (boolVal true) := by simpa [h7] using evEq 5126 (by decide)
                rw [spec4 (Or.inr (Or.inr h7))]
                exact ExS.ite (bc := true) hc7 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
              · have hc7 : EvE Gen.prog ⟨loc, m, false⟩
                    (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5126)))
                    (boolVal false) := by simpa [h7] using evEq 5126 (by decide)
                apply ExS.ite (bc := false) hc7 (by simp [truthy, boolVal])
                by_cases h8 : n = 5130
                · have hc8 : EvE Gen.prog ⟨loc, m, false⟩
                      (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5130)))
                      (boolVal true) := by simpa [h8] using evEq 5130 (by decide)
                  rw [spec8 h8]
                  exact ExS.ite (bc := true) hc8 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
                · have hc8 : EvE Gen.prog ⟨loc, m, false⟩
                      (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 5130)))
                      (boolVal false) := by simpa [h8] using evEq 5130 (by decide)
                  rw [specM h1 h2 h3 h4 h5 h6 h7 h8]
                  apply ExS.ite (bc := false) hc8 (by simp [truthy, boolVal])
                  exact ExS.ret (evNegOneI32 loc m false)

theorem num_components_correct (loc : Locals) (m : Mem) (n : Nat)
    (h0 : loc 0 = some (.int IntTy.i32 n)) (hn : n < 4294967296) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3_num_components.body
      (.ret (some (.int IntTy.i32 (GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n))))))
      ⟨loc, m, false⟩ := by
  have evEq (c : Nat) (hc : c < 2147483648) :
      EvE Gen.prog ⟨loc, m, false⟩
        (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 c)))
        (boolVal (n = c)) :=
    EvE.bin (EvE.var h0) (EvE.const _) (evalBin_eq_i32 n c hn hc)
  have spec1 (h : n = 65) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 1 := by
    subst h; decide
  have spec2 (h : n = 2) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 2 := by
    subst h; decide
  have spec3 (h : n = 3) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 3 := by
    subst h; decide
  have spec4a (h : n = 4) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 4 := by
    subst h; decide
  have spec4b (h : n = 34) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 4 := by
    subst h; decide
  have spec9 (h : n = 35) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 9 := by
    subst h; decide
  have spec16 (h : n = 36) : GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 16 := by
    subst h; decide
  have specM (h1 : n ≠ 65) (h2 : n ≠ 2) (h3 : n ≠ 3) (h4 : n ≠ 4)
      (h5 : n ≠ 34) (h6 : n ≠ 35) (h7 : n ≠ 36) :
      GlTF.wrap32 (GlTF.numComponents (Int32.ofNat n)) = 4294967295 := by
    have e1 : Int32.ofNat n ≠ (65 : Int32) := by simpa using int32_ofNat_ne n 65 hn (by decide) h1
    have e2 : Int32.ofNat n ≠ (2 : Int32) := by simpa using int32_ofNat_ne n 2 hn (by decide) h2
    have e3 : Int32.ofNat n ≠ (3 : Int32) := by simpa using int32_ofNat_ne n 3 hn (by decide) h3
    have e4 : Int32.ofNat n ≠ (4 : Int32) := by simpa using int32_ofNat_ne n 4 hn (by decide) h4
    have e5 : Int32.ofNat n ≠ (34 : Int32) := by simpa using int32_ofNat_ne n 34 hn (by decide) h5
    have e6 : Int32.ofNat n ≠ (35 : Int32) := by simpa using int32_ofNat_ne n 35 hn (by decide) h6
    have e7 : Int32.ofNat n ≠ (36 : Int32) := by simpa using int32_ofNat_ne n 36 hn (by decide) h7
    simp [GlTF.numComponents, GlTF.AT.scalar, GlTF.AT.vec2, GlTF.AT.vec3, GlTF.AT.vec4,
      GlTF.AT.mat2, GlTF.AT.mat3, GlTF.AT.mat4, e1, e2, e3, e4, e5, e6, e7, GlTF.wrap32]
  simp only [Gen.tg3_num_components]
  by_cases h1 : n = 65
  · have hc : EvE Gen.prog ⟨loc, m, false⟩
        (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 65)))
        (boolVal true) := by simpa [h1] using evEq 65 (by decide)
    rw [spec1 h1]
    exact ExS.ite (bc := true) hc (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
  · have hc : EvE Gen.prog ⟨loc, m, false⟩
        (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 65)))
        (boolVal false) := by simpa [h1] using evEq 65 (by decide)
    apply ExS.ite (bc := false) hc (by simp [truthy, boolVal])
    by_cases h2 : n = 2
    · have hc2 : EvE Gen.prog ⟨loc, m, false⟩
          (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 2)))
          (boolVal true) := by simpa [h2] using evEq 2 (by decide)
      rw [spec2 h2]
      exact ExS.ite (bc := true) hc2 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
    · have hc2 : EvE Gen.prog ⟨loc, m, false⟩
          (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 2)))
          (boolVal false) := by simpa [h2] using evEq 2 (by decide)
      apply ExS.ite (bc := false) hc2 (by simp [truthy, boolVal])
      by_cases h3 : n = 3
      · have hc3 : EvE Gen.prog ⟨loc, m, false⟩
            (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 3)))
            (boolVal true) := by simpa [h3] using evEq 3 (by decide)
        rw [spec3 h3]
        exact ExS.ite (bc := true) hc3 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
      · have hc3 : EvE Gen.prog ⟨loc, m, false⟩
            (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 3)))
            (boolVal false) := by simpa [h3] using evEq 3 (by decide)
        apply ExS.ite (bc := false) hc3 (by simp [truthy, boolVal])
        by_cases h4 : n = 4
        · have hc4 : EvE Gen.prog ⟨loc, m, false⟩
              (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 4)))
              (boolVal true) := by simpa [h4] using evEq 4 (by decide)
          rw [spec4a h4]
          exact ExS.ite (bc := true) hc4 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
        · have hc4 : EvE Gen.prog ⟨loc, m, false⟩
              (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 4)))
              (boolVal false) := by simpa [h4] using evEq 4 (by decide)
          apply ExS.ite (bc := false) hc4 (by simp [truthy, boolVal])
          by_cases h5 : n = 34
          · have hc5 : EvE Gen.prog ⟨loc, m, false⟩
                (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 34)))
                (boolVal true) := by simpa [h5] using evEq 34 (by decide)
            rw [spec4b h5]
            exact ExS.ite (bc := true) hc5 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
          · have hc5 : EvE Gen.prog ⟨loc, m, false⟩
                (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 34)))
                (boolVal false) := by simpa [h5] using evEq 34 (by decide)
            apply ExS.ite (bc := false) hc5 (by simp [truthy, boolVal])
            by_cases h6 : n = 35
            · have hc6 : EvE Gen.prog ⟨loc, m, false⟩
                  (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 35)))
                  (boolVal true) := by simpa [h6] using evEq 35 (by decide)
              rw [spec9 h6]
              exact ExS.ite (bc := true) hc6 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
            · have hc6 : EvE Gen.prog ⟨loc, m, false⟩
                  (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 35)))
                  (boolVal false) := by simpa [h6] using evEq 35 (by decide)
              apply ExS.ite (bc := false) hc6 (by simp [truthy, boolVal])
              by_cases h7 : n = 36
              · have hc7 : EvE Gen.prog ⟨loc, m, false⟩
                    (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 36)))
                    (boolVal true) := by simpa [h7] using evEq 36 (by decide)
                rw [spec16 h7]
                exact ExS.ite (bc := true) hc7 (by simp [truthy, boolVal]) (ExS.ret (EvE.const _))
              · have hc7 : EvE Gen.prog ⟨loc, m, false⟩
                    (Expr.bin .eq (some IntTy.i32) (.var 0) (Expr.const (Val.int IntTy.i32 36)))
                    (boolVal false) := by simpa [h7] using evEq 36 (by decide)
                rw [specM h1 h2 h3 h4 h5 h6 h7]
                apply ExS.ite (bc := false) hc7 (by simp [truthy, boolVal])
                exact ExS.ret (evNegOneI32 loc m false)

theorem charValueU_ofNat (n : Nat) (hn : n < 256) :
    GlTF.B64.charValueU (UInt8.ofNat n) =
      (if 65 ≤ n ∧ n ≤ 90 then n - 65
       else if 97 ≤ n ∧ n ≤ 122 then n - 97 + 26
       else if 48 ≤ n ∧ n ≤ 57 then n - 48 + 52
       else if n = 43 then 62
       else if n = 47 then 63
       else 4294967295) := by
  simp [GlTF.B64.charValueU, UInt8.toNat_ofNat, Nat.mod_eq_of_lt hn]

theorem evCastU8I32 (loc : Locals) (m : Mem) (ro : Bool) (n : Nat) (hn : n < 256)
    (h0 : loc 0 = some (.int IntTy.u8 n)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.i32 (.var 0)) (.int IntTy.i32 n) := by
  refine EvE.cast (EvE.var h0) ?_
  simp [evalCast, IntTy.u8, IntTy.i32, IntTy.modulus, Nat.mod_eq_of_lt (by omega : n < 4294967296)]

theorem evalBin_ge_i32 (a c : Nat) (ha : a < 2147483648) (hc : c < 2147483648) :
    evalBin .ge (some IntTy.i32) (.int IntTy.i32 a) (.int IntTy.i32 c)
      = .ok (boolVal (a ≥ c)) := by
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.i32, IntTy.toInt, IntTy.modulus,
    Nat.mod_eq_of_lt (by omega : a < 4294967296), Nat.mod_eq_of_lt (by omega : c < 4294967296),
    boolVal]
  by_cases h : c ≤ a
  · have ha' : ¬ 2147483648 ≤ a := by omega
    have hc' : ¬ 2147483648 ≤ c := by omega
    simp [h, ha', hc']
  · have h' : ¬ (a ≥ c) := by omega
    have ha' : ¬ 2147483648 ≤ a := by omega
    have hc' : ¬ 2147483648 ≤ c := by omega
    simp [h', h, ha', hc']

theorem evalBin_le_i32 (a c : Nat) (ha : a < 2147483648) (hc : c < 2147483648) :
    evalBin .le (some IntTy.i32) (.int IntTy.i32 a) (.int IntTy.i32 c)
      = .ok (boolVal (a ≤ c)) := by
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.i32, IntTy.toInt, IntTy.modulus,
    Nat.mod_eq_of_lt (by omega : a < 4294967296), Nat.mod_eq_of_lt (by omega : c < 4294967296),
    boolVal]
  by_cases h : a ≤ c
  · have ha' : ¬ 2147483648 ≤ a := by omega
    have hc' : ¬ 2147483648 ≤ c := by omega
    simp [h, ha', hc']
  · have h' : ¬ (a ≤ c) := h
    have ha' : ¬ 2147483648 ≤ a := by omega
    have hc' : ¬ 2147483648 ≤ c := by omega
    simp [h', ha', hc']

theorem int_sub_mod_toNat (a b : Nat) (hb : b ≤ a) (ha : a < 4294967296) :
    (((a : Int) - (b : Int)) % 4294967296).toNat = a - b := by
  have hsub : ((a : Int) - (b : Int)) = (a - b : Nat) := by omega
  rw [hsub, Int.emod_eq_of_lt]
  · rw [Int.toNat_natCast]
  · omega
  · omega

theorem int_add_mod_toNat (a b : Nat) (h : a + b < 4294967296) :
    (((a : Int) + (b : Int)) % 4294967296).toNat = a + b := by
  have hsub : ((a : Int) + (b : Int)) = (a + b : Nat) := by omega
  rw [hsub, Int.emod_eq_of_lt]
  · rw [Int.toNat_natCast]
  · omega
  · omega

theorem evSubI32 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.i32 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.i32 b))
    (hb_le : b ≤ a) (ha31 : a < 2147483648) (hb31 : b < 2147483648) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .sub (some IntTy.i32) aE bE) (.int IntTy.i32 (a - b)) := by
  refine EvE.bin ha hb ?_
  have ha' : ¬ 2147483648 ≤ a := by omega
  have hb' : ¬ 2147483648 ≤ b := by omega
  have h1 : (-2147483648 : Int) ≤ (a : Int) - (b : Int) := by omega
  have h2 : (a : Int) - (b : Int) ≤ 2147483647 := by omega
  have hw : (a - b) % 4294967296 = a - b := Nat.mod_eq_of_lt (by omega)
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.i32, IntTy.toInt, IntTy.modulus,
    ha', hb', arith, IntTy.inRange, IntTy.minVal, IntTy.maxVal, IntTy.wrap, h1, h2, hw, int_sub_mod_toNat a b hb_le (by omega)]

theorem evAddI32 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.i32 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.i32 b))
    (h : a + b < 2147483648) (ha31 : a < 2147483648) (hb31 : b < 2147483648) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .add (some IntTy.i32) aE bE) (.int IntTy.i32 (a + b)) := by
  refine EvE.bin ha hb ?_
  have ha' : ¬ 2147483648 ≤ a := by omega
  have hb' : ¬ 2147483648 ≤ b := by omega
  have h1 : (-2147483648 : Int) ≤ (a : Int) + (b : Int) := by omega
  have h2 : (a : Int) + (b : Int) ≤ 2147483647 := by omega
  have hw : (a + b) % 4294967296 = a + b := Nat.mod_eq_of_lt (by omega)
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.i32, IntTy.toInt, IntTy.modulus,
    ha', hb', arith, IntTy.inRange, IntTy.minVal, IntTy.maxVal, IntTy.wrap, h1, h2, hw, int_add_mod_toNat a b (by omega)]

theorem charValueU_eq1 (n : Nat) (hn : n < 256)
    (h1a : 65 ≤ n) (h1b : n ≤ 90) :
    GlTF.B64.charValueU (UInt8.ofNat n) = n - 65 := by
  rw [charValueU_ofNat n hn]; simp [h1a, h1b]

theorem charValueU_eq2 (n : Nat) (hn : n < 256)
    (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2a : 97 ≤ n) (h2b : n ≤ 122) :
    GlTF.B64.charValueU (UInt8.ofNat n) = n - 97 + 26 := by
  rw [charValueU_ofNat n hn]; simp [h1, h2a, h2b]

theorem charValueU_eq3 (n : Nat) (hn : n < 256)
    (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2 : ¬ (97 ≤ n ∧ n ≤ 122))
    (h3a : 48 ≤ n) (h3b : n ≤ 57) :
    GlTF.B64.charValueU (UInt8.ofNat n) = n - 48 + 52 := by
  rw [charValueU_ofNat n hn]; simp [h1, h2, h3a, h3b]

theorem charValueU_eq4 (n : Nat) (hn : n < 256)
    (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2 : ¬ (97 ≤ n ∧ n ≤ 122))
    (h3 : ¬ (48 ≤ n ∧ n ≤ 57)) (h4 : n = 43) :
    GlTF.B64.charValueU (UInt8.ofNat n) = 62 := by
  rw [charValueU_ofNat n hn]; simp [h1, h2, h3, h4]

theorem charValueU_eq5 (n : Nat) (hn : n < 256)
    (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2 : ¬ (97 ≤ n ∧ n ≤ 122))
    (h3 : ¬ (48 ≤ n ∧ n ≤ 57)) (h4 : n ≠ 43) (h5 : n = 47) :
    GlTF.B64.charValueU (UInt8.ofNat n) = 63 := by
  rw [charValueU_ofNat n hn]; simp [h1, h2, h3, h4, h5]

theorem charValueU_eqM (n : Nat) (hn : n < 256)
    (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2 : ¬ (97 ≤ n ∧ n ≤ 122))
    (h3 : ¬ (48 ≤ n ∧ n ≤ 57)) (h4 : n ≠ 43) (h5 : n ≠ 47) :
    GlTF.B64.charValueU (UInt8.ofNat n) = 4294967295 := by
  rw [charValueU_ofNat n hn]; simp [h1, h2, h3, h4, h5]


def b64cE : Expr := .cast IntTy.i32 (.var 0)

def b64c1 : Stmt := .ite
  (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 65)))
         (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 90))))
  (.ret (some (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 65))))) .skip

def b64c2 : Stmt := .ite
  (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
         (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 122))))
  (.ret (some (.bin .add (some IntTy.i32)
    (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
    (.const (.int IntTy.i32 26))))) .skip

def b64c3 : Stmt := .ite
  (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
         (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 57))))
  (.ret (some (.bin .add (some IntTy.i32)
    (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
    (.const (.int IntTy.i32 52))))) .skip

def b64c4 : Stmt := .ite
  (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 43)))
  (.ret (some (.const (.int IntTy.i32 62)))) .skip

def b64c5 : Stmt := .ite
  (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 47)))
  (.ret (some (.const (.int IntTy.i32 63)))) .skip

def b64ret : Stmt := .ret (some (.un .neg IntTy.i32 (.const (.int IntTy.i32 1))))

def b64rest4 : Stmt := .seq b64c4 (.seq b64c5 b64ret)
def b64rest3 : Stmt := .seq b64c3 b64rest4
def b64rest2 : Stmt := .seq b64c2 b64rest3
def b64body : Stmt := .seq b64c1 b64rest2

theorem b64_body_eq : Gen.tg3__b64_decode_char.body = b64body := rfl

theorem b64_decode_char_correct (loc : Locals) (m : Mem) (n : Nat)
    (h0 : loc 0 = some (.int IntTy.u8 n)) (hn : n < 256) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__b64_decode_char.body
      (.ret (some (.int IntTy.i32 (GlTF.B64.charValueU (UInt8.ofNat n)))))
      ⟨loc, m, false⟩ := by
  have hcE : EvE Gen.prog ⟨loc, m, false⟩ b64cE (.int IntTy.i32 n) :=
    evCastU8I32 loc m false n hn h0
  have hge (c : Nat) (hc : c < 2147483648) :
      EvE Gen.prog ⟨loc, m, false⟩
        (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 c))) (boolVal (n ≥ c)) :=
    EvE.bin hcE (EvE.const _) (evalBin_ge_i32 n c (by omega) hc)
  have hle (c : Nat) (hc : c < 2147483648) :
      EvE Gen.prog ⟨loc, m, false⟩
        (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 c))) (boolVal (n ≤ c)) :=
    EvE.bin hcE (EvE.const _) (evalBin_le_i32 n c (by omega) hc)
  have heq (c : Nat) (hc : c < 2147483648) :
      EvE Gen.prog ⟨loc, m, false⟩
        (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 c))) (boolVal (n = c)) :=
    EvE.bin hcE (EvE.const _) (evalBin_eq_i32 n c (by omega) hc)
  have rest4 (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2 : ¬ (97 ≤ n ∧ n ≤ 122))
      (h3 : ¬ (48 ≤ n ∧ n ≤ 57)) :
      ExS Gen.prog ⟨loc, m, false⟩ b64rest4
        (.ret (some (.int IntTy.i32 (GlTF.B64.charValueU (UInt8.ofNat n)))))
        ⟨loc, m, false⟩ := by
    by_cases h4 : n = 43
    · have hcond : EvE Gen.prog ⟨loc, m, false⟩
          (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 43))) (boolVal true) :=
        by simpa [b64cE, h4] using heq 43 (by decide)
      rw [charValueU_eq4 n hn h1 h2 h3 h4]
      exact ExS.seq_ret (ExS.ite (bc := true) hcond (by simp [truthy, boolVal])
        (ExS.ret (EvE.const _)))
    · have hcond : EvE Gen.prog ⟨loc, m, false⟩
          (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 43))) (boolVal false) :=
        by simpa [b64cE, h4] using heq 43 (by decide)
      apply ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
      by_cases h5 : n = 47
      · have hcond5 : EvE Gen.prog ⟨loc, m, false⟩
            (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 47))) (boolVal true) :=
          by simpa [b64cE, h5] using heq 47 (by decide)
        rw [charValueU_eq5 n hn h1 h2 h3 h4 h5]
        exact ExS.seq_ret (ExS.ite (bc := true) hcond5 (by simp [truthy, boolVal])
          (ExS.ret (EvE.const _)))
      · have hcond5 : EvE Gen.prog ⟨loc, m, false⟩
            (.bin .eq (some IntTy.i32) b64cE (.const (.int IntTy.i32 47))) (boolVal false) :=
          by simpa [b64cE, h5] using heq 47 (by decide)
        apply ExS.seq (ExS.ite (bc := false) hcond5 (by simp [truthy, boolVal]) ExS.skip)
        rw [charValueU_eqM n hn h1 h2 h3 h4 h5]
        exact ExS.ret (evNegOneI32 loc m false)
  have rest3 (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) (h2 : ¬ (97 ≤ n ∧ n ≤ 122)) :
      ExS Gen.prog ⟨loc, m, false⟩ b64rest3
        (.ret (some (.int IntTy.i32 (GlTF.B64.charValueU (UInt8.ofNat n)))))
        ⟨loc, m, false⟩ := by
    by_cases h3a : 48 ≤ n
    · by_cases h3b : n ≤ 57
      · have hcond : EvE Gen.prog ⟨loc, m, false⟩
            (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
              (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 57)))) (boolVal true) :=
          EvE.land_true (hge 48 (by decide)) (by simp [truthy, boolVal, h3a])
            (hle 57 (by decide)) (by simp [truthy, boolVal, h3b])
        have hsub : EvE Gen.prog ⟨loc, m, false⟩
            (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
            (.int IntTy.i32 (n - 48)) :=
          evSubI32 loc m false n 48 b64cE (.const (.int IntTy.i32 48)) hcE (EvE.const _)
            h3a (by omega) (by decide)
        have hadd : EvE Gen.prog ⟨loc, m, false⟩
            (.bin .add (some IntTy.i32)
              (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
              (.const (.int IntTy.i32 52)))
            (.int IntTy.i32 (n - 48 + 52)) :=
          evAddI32 loc m false (n - 48) 52 _ (.const (.int IntTy.i32 52)) hsub (EvE.const _)
            (by omega) (by omega) (by decide)
        rw [charValueU_eq3 n hn h1 h2 h3a h3b]
        exact ExS.seq_ret (ExS.ite (bc := true) hcond (by simp [truthy, boolVal])
          (ExS.ret hadd))
      · have h3 : ¬ (48 ≤ n ∧ n ≤ 57) := by intro h; exact h3b h.2
        have hcond : EvE Gen.prog ⟨loc, m, false⟩
            (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
              (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 57)))) (boolVal false) :=
          EvE.land_true (hge 48 (by decide)) (by simp [truthy, boolVal, h3a])
            (hle 57 (by decide)) (by simp [truthy, boolVal, h3b])
        exact ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
          (rest4 h1 h2 h3)
    · have h3 : ¬ (48 ≤ n ∧ n ≤ 57) := by intro h; exact h3a h.1
      have hcond : EvE Gen.prog ⟨loc, m, false⟩
          (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 48)))
            (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 57)))) (boolVal false) :=
        EvE.land_false (hge 48 (by decide)) (by simp [truthy, boolVal, h3a])
      exact ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
        (rest4 h1 h2 h3)
  have rest2 (h1 : ¬ (65 ≤ n ∧ n ≤ 90)) :
      ExS Gen.prog ⟨loc, m, false⟩ b64rest2
        (.ret (some (.int IntTy.i32 (GlTF.B64.charValueU (UInt8.ofNat n)))))
        ⟨loc, m, false⟩ := by
    by_cases h2a : 97 ≤ n
    · by_cases h2b : n ≤ 122
      · have hcond : EvE Gen.prog ⟨loc, m, false⟩
            (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
              (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 122)))) (boolVal true) :=
          EvE.land_true (hge 97 (by decide)) (by simp [truthy, boolVal, h2a])
            (hle 122 (by decide)) (by simp [truthy, boolVal, h2b])
        have hsub : EvE Gen.prog ⟨loc, m, false⟩
            (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
            (.int IntTy.i32 (n - 97)) :=
          evSubI32 loc m false n 97 b64cE (.const (.int IntTy.i32 97)) hcE (EvE.const _)
            h2a (by omega) (by decide)
        have hadd : EvE Gen.prog ⟨loc, m, false⟩
            (.bin .add (some IntTy.i32)
              (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
              (.const (.int IntTy.i32 26)))
            (.int IntTy.i32 (n - 97 + 26)) :=
          evAddI32 loc m false (n - 97) 26 _ (.const (.int IntTy.i32 26)) hsub (EvE.const _)
            (by omega) (by omega) (by decide)
        rw [charValueU_eq2 n hn h1 h2a h2b]
        exact ExS.seq_ret (ExS.ite (bc := true) hcond (by simp [truthy, boolVal])
          (ExS.ret hadd))
      · have h2 : ¬ (97 ≤ n ∧ n ≤ 122) := by intro h; exact h2b h.2
        have hcond : EvE Gen.prog ⟨loc, m, false⟩
            (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
              (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 122)))) (boolVal false) :=
          EvE.land_true (hge 97 (by decide)) (by simp [truthy, boolVal, h2a])
            (hle 122 (by decide)) (by simp [truthy, boolVal, h2b])
        exact ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
          (rest3 h1 h2)
    · have h2 : ¬ (97 ≤ n ∧ n ≤ 122) := by intro h; exact h2a h.1
      have hcond : EvE Gen.prog ⟨loc, m, false⟩
          (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 97)))
            (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 122)))) (boolVal false) :=
        EvE.land_false (hge 97 (by decide)) (by simp [truthy, boolVal, h2a])
      exact ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
        (rest3 h1 h2)
  rw [b64_body_eq]
  by_cases h1a : 65 ≤ n
  · by_cases h1b : n ≤ 90
    · have hcond : EvE Gen.prog ⟨loc, m, false⟩
          (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 65)))
            (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 90)))) (boolVal true) :=
        EvE.land_true (hge 65 (by decide)) (by simp [truthy, boolVal, h1a])
          (hle 90 (by decide)) (by simp [truthy, boolVal, h1b])
      have hsub : EvE Gen.prog ⟨loc, m, false⟩
          (.bin .sub (some IntTy.i32) b64cE (.const (.int IntTy.i32 65)))
          (.int IntTy.i32 (n - 65)) :=
        evSubI32 loc m false n 65 b64cE (.const (.int IntTy.i32 65)) hcE (EvE.const _)
          h1a (by omega) (by decide)
      rw [charValueU_eq1 n hn h1a h1b]
      exact ExS.seq_ret (ExS.ite (bc := true) hcond (by simp [truthy, boolVal])
        (ExS.ret hsub))
    · have h1 : ¬ (65 ≤ n ∧ n ≤ 90) := by intro h; exact h1b h.2
      have hcond : EvE Gen.prog ⟨loc, m, false⟩
          (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 65)))
            (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 90)))) (boolVal false) :=
        EvE.land_true (hge 65 (by decide)) (by simp [truthy, boolVal, h1a])
          (hle 90 (by decide)) (by simp [truthy, boolVal, h1b])
      exact ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
        (rest2 h1)
  · have h1 : ¬ (65 ≤ n ∧ n ≤ 90) := by intro h; exact h1a h.1
    have hcond : EvE Gen.prog ⟨loc, m, false⟩
        (.land (.bin .ge (some IntTy.i32) b64cE (.const (.int IntTy.i32 65)))
          (.bin .le (some IntTy.i32) b64cE (.const (.int IntTy.i32 90)))) (boolVal false) :=
      EvE.land_false (hge 65 (by decide)) (by simp [truthy, boolVal, h1a])
    exact ExS.seq (ExS.ite (bc := false) hcond (by simp [truthy, boolVal]) ExS.skip)
      (rest2 h1)

end GlTF.C.Proofs
