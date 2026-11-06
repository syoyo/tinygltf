/-
  T1 (write): the GLB header writers.

  `hdr32` / `hdr64` describe the exact sequence of `splice`s the C performs,
  so the function-level theorems are "the C performs these writes, in this
  order, with these arguments, without undefined behaviour".
-/
import GlTF.C.Proofs.Wr64
import GlTF.C.Proofs.GlbParse

namespace GlTF.C.Proofs
open GlTF GlTF.C

set_option linter.unusedSimpArgs false
set_option linter.deprecated false

/-! ## Frame object access -/

/-- The fields a writer reads from the frame object, as byte slices. -/
structure FrameAt (fblk : Block) (f : Glb.Frame) : Prop where
  joff : List.take 8 (List.drop 0 fblk) = encodeLE 8 f.jsonOff
  jlen : List.take 8 (List.drop 8 fblk) = encodeLE 8 f.jsonLen
  boff : List.take 8 (List.drop 16 fblk) = encodeLE 8 f.binOff
  blen : List.take 8 (List.drop 24 fblk) = encodeLE 8 f.binLen
  hjs : List.take 4 (List.drop 32 fblk) = encodeLE 4 (b2n f.hasJson)
  hbn : List.take 4 (List.drop 36 fblk) = encodeLE 4 (b2n f.hasBin)
  is64 : List.take 4 (List.drop 40 fblk) = encodeLE 4 (b2n f.is64)
  len : 44 ≤ fblk.length

/-- A field read from the frame object. -/
theorem frameLoad (loc : Locals) (m : Mem) (ro : Bool) (bf b : Nat) (fblk : Block)
    (hp : loc 1 = some (.ptr bf 0)) (hm : m bf = some fblk)
    (o : Nat) (t : IntTy) (w : Nat) (hw : t.width / 8 = w) (ho : o + w ≤ fblk.length)
    (v : Nat) (hv : v < 256 ^ w)
    (he : List.take w (List.drop o fblk) = encodeLE w v) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.load (Ty.int t) (.field (.var 1) o))
      (.int t v) := by
  refine EvE.of_eq 25 (fun n => ?_)
  simp [Gen.prog, evalExpr, evalBin, evalBinInt, binInt, load, readBytes, ptrAddN, decodeLE_encodeLE,
    IntTy.toInt, IntTy.bytes, Ty.size, Nat.mod_eq_of_lt, isPtr, ptrEq, C.guard, hp, hm,
    show o ≤ fblk.length by omega, ho, hw, hv, Nat.mod_eq_of_lt hv, he,
    show t.width / 8 = w from hw]

end GlTF.C.Proofs

namespace GlTF.C.Proofs

/-- `tg3__glb_align4` is `Glb.align4`. -/
theorem align4_correct (loc : Locals) (m : Mem) (ro : Bool) (n : Nat)
    (h0 : loc 0 = some (.int IntTy.u64 n)) (hn : n + 3 < 18446744073709551616) :
    ExS Gen.prog ⟨loc, m, ro⟩ Gen.tg3__glb_align4.body
      (.ret (some (.int IntTy.u64 (Glb.align4 n)))) ⟨loc, m, ro⟩ := by
  refine ExS.of_eq 25 (fun _ => ?_)
  simp [Gen.tg3__glb_align4, execStmt, evalExpr, evalBin, evalBinInt, binInt, evalCast, intOf,
    IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt, isPtr, C.guard, h0, hn,
    Glb.align4, (by omega : (n + 3) / 4 * 4 < 18446744073709551616)]

/-! ## Magic -/

/-- Four adjacent single-byte splices are one 4-byte splice. -/
theorem splice_bytes4 (blk : Block) (o : Nat) (h : o + 4 ≤ blk.length) :
    splice (splice (splice (splice blk o [103]) (o + 1) [108]) (o + 2) [84]) (o + 3) [70]
      = splice blk o [103, 108, 84, 70] := by
  have e1 : splice (splice blk o [103]) (o + 1) [108] = splice blk o ([103] ++ [108]) :=
    splice_append blk o _ _ (by simp only [List.length_cons, List.length_nil]; omega)
  have e2 : splice (splice blk o ([103] ++ [108])) (o + 2) [84]
      = splice blk o (([103] ++ [108]) ++ [84]) :=
    splice_append blk o _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  have e3 : splice (splice blk o (([103] ++ [108]) ++ [84])) (o + 3) [70]
      = splice blk o ((([103] ++ [108]) ++ [84]) ++ [70]) :=
    splice_append blk o _ _ (by simp only [List.length_cons, List.length_nil, List.length_append]; omega)
  rw [e1, e2, e3]
  rfl

/-- A single magic byte store at index 0..3. -/
theorem magicByte (loc : Locals) (m : Mem) (bo : Nat) (blk : Block) (hm : m bo = some blk)
    (h0 : loc 0 = some (.ptr bo 0)) (o v : Nat) (hov : o = 0 ∨ o = 1 ∨ o = 2 ∨ o = 3)
    (hv : v < 256) (hb : o + 1 ≤ blk.length) :
    ExS Gen.prog ⟨loc, m, false⟩
      (.store (Ty.int IntTy.u8) (.index (.var 0) (.const (.int IntTy.i32 o)) 1)
        (.cast IntTy.u8 (.const (.int IntTy.u32 v))))
      .normal ⟨loc, m.upd bo (splice blk o [v.toUInt8]), false⟩ := by
  have ho0 : ¬ 2147483648 ≤ o := by omega
  have hi1 : (0 : Int) ≤ (o : Int) := by omega
  have hi2 : (o : Int) ≤ (blk.length : Int) := by omega
  refine ExS.of_eq 25 (fun _ => ?_)
  wsimp [h0, hm, hv, hb, ptrAdd, ho0, hi1, hi2]
  simp [encodeLE, splice, List.append_assoc, Nat.mod_eq_of_lt hv]

/-- **`tg3__glb_write_magic`** writes `"glTF"` at the start of `out`. -/
theorem write_magic_writes (loc : Locals) (m : Mem) (bo : Nat) (blk : Block)
    (hm : m bo = some blk) (h0 : loc 0 = some (.ptr bo 0)) (h4 : 4 ≤ blk.length) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_magic.body .normal
      ⟨loc, m.upd bo (splice blk 0 [103, 108, 84, 70]), false⟩ := by
  have len1 : (splice blk 0 [103]).length = blk.length :=
    splice_length blk 0 _ (by show 0 + 1 ≤ blk.length; omega)
  have len2 : (splice (splice blk 0 [103]) 1 [108]).length = blk.length := by
    rw [splice_length _ _ _ (by rw [len1]; show 1 + 1 ≤ blk.length; omega), len1]
  have len3 : (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84]).length = blk.length := by
    rw [splice_length _ _ _ (by rw [len2]; show 2 + 1 ≤ blk.length; omega), len2]
  simp only [Gen.tg3__glb_write_magic]
  have s1 := magicByte loc m bo blk hm h0 0 103 (Or.inl rfl) (by decide)
    (by show 0 + 1 ≤ blk.length; omega)
  have s2 : ExS Gen.prog ⟨loc, m.upd bo (splice blk 0 [103]), false⟩
      (.store (Ty.int IntTy.u8) (.index (.var 0) (.const (.int IntTy.i32 1)) 1)
        (.cast IntTy.u8 (.const (.int IntTy.u32 108))))
      .normal ⟨loc, m.upd bo (splice (splice blk 0 [103]) 1 [108]), false⟩ := by
    simpa [Mem.upd_upd] using magicByte loc (m.upd bo (splice blk 0 [103])) bo (splice blk 0 [103])
      (by simp) h0 1 108 (Or.inr (Or.inl rfl)) (by decide)
      (by rw [len1]; show 1 + 1 ≤ blk.length; omega)
  have s3 : ExS Gen.prog ⟨loc, m.upd bo (splice (splice blk 0 [103]) 1 [108]), false⟩
      (.store (Ty.int IntTy.u8) (.index (.var 0) (.const (.int IntTy.i32 2)) 1)
        (.cast IntTy.u8 (.const (.int IntTy.u32 84))))
      .normal ⟨loc, m.upd bo (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84]), false⟩ := by
    simpa [Mem.upd_upd] using magicByte loc (m.upd bo (splice (splice blk 0 [103]) 1 [108])) bo
      (splice (splice blk 0 [103]) 1 [108]) (by simp) h0 2 84 (Or.inr (Or.inr (Or.inl rfl)))
      (by decide) (by rw [len2]; show 2 + 1 ≤ blk.length; omega)
  have s4 : ExS Gen.prog
      ⟨loc, m.upd bo (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84]), false⟩
      (.store (Ty.int IntTy.u8) (.index (.var 0) (.const (.int IntTy.i32 3)) 1)
        (.cast IntTy.u8 (.const (.int IntTy.u32 70))))
      .normal ⟨loc, m.upd bo (splice (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84]) 3 [70]),
        false⟩ := by
    simpa [Mem.upd_upd] using magicByte loc
      (m.upd bo (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84])) bo
      (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84]) (by simp) h0 3 70
      (Or.inr (Or.inr (Or.inr rfl))) (by decide)
      (by rw [len3]; show 3 + 1 ≤ blk.length; omega)
  have hchain := ExS.seq s1 (ExS.seq s2 (ExS.seq s3 s4))
  have hs : (⟨loc, m.upd bo (splice (splice (splice (splice blk 0 [103]) 1 [108]) 2 [84]) 3 [70]),
      false⟩ : State) = ⟨loc, m.upd bo (splice blk 0 [103, 108, 84, 70]), false⟩ := by
    rw [splice_bytes4 blk 0 h4]
  simpa [hs, Mem.upd_upd, IntTy.u8, IntTy.i32, IntTy.u32] using hchain

end GlTF.C.Proofs
