/-
  T1 (write): `tg3__glb_write_header32` and `tg3__glb_write_header64`.
-/
import GlTF.C.Proofs.Hdr
import GlTF.C.Proofs.Wr32
import GlTF.C.Proofs.Wr64
import GlTF.C.Proofs.Loop
import GlTF.C.Splice
import GlTF.C.Lemmas

namespace GlTF.C.Proofs
open GlTF GlTF.C GlTF.Glb

set_option linter.unusedSimpArgs false
set_option linter.deprecated false
set_option maxRecDepth 100000
set_option maxHeartbeats 10000000

/-! ## Reusable step lemmas -/

/-- A call `tg3__wr_u32le(p, off, v)` as a write of `encodeLE 4 v` at `off`. -/
theorem wr32_call (loc : Locals) (m : Mem) (bo : Nat) (blk : Block) (hm : m bo = some blk)
    (off v : Nat) (offE valE : Expr) (h0 : loc 0 = some (.ptr bo 0))
    (hoff : EvE Gen.prog ⟨loc, m, false⟩ offE (.int IntTy.u64 off))
    (hval : EvE Gen.prog ⟨loc, m, false⟩ valE (.int IntTy.u32 v))
    (hlt : off + 4 ≤ blk.length) (hbl : blk.length < 18446744073709551616)
    (hvv : v < 4294967296) :
    ExS Gen.prog ⟨loc, m, false⟩ (.call none 4 [.var 0, offE, valE]) .normal
      ⟨loc, m.upd bo (splice blk off (encodeLE 4 v)), false⟩ := by
  have hargs : EvArgs Gen.prog ⟨loc, m, false⟩ [.var 0, offE, valE]
      [.ptr bo 0, .int IntTy.u64 off, .int IntTy.u32 v] :=
    EvArgs.cons (EvE.var h0) (EvArgs.cons hoff (EvArgs.cons hval EvArgs.nil))
  have body : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 off, .int IntTy.u32 v], m, false⟩
      Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 off, .int IntTy.u32 v],
        m.upd bo (splice blk off (encodeLE 4 v)), false⟩ :=
    wr_u32le_writes _ m bo blk hm off v (frameLocals0 _ _ _) (frameLocals1 _ _ _)
      (frameLocals2 _ _ _) hlt hbl hvv
  exact ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs rfl rfl body (Or.inl rfl)

/-- A call `tg3__wr_u64le(p, off, v)` as a write of `encodeLE 8 v` at `off`. -/
theorem wr64_call (loc : Locals) (m : Mem) (bo : Nat) (blk : Block) (hm : m bo = some blk)
    (off v : Nat) (offE valE : Expr) (h0 : loc 0 = some (.ptr bo 0))
    (hoff : EvE Gen.prog ⟨loc, m, false⟩ offE (.int IntTy.u64 off))
    (hval : EvE Gen.prog ⟨loc, m, false⟩ valE (.int IntTy.u64 v))
    (hlt : off + 8 ≤ blk.length) (hbl : blk.length < 18446744073709551616)
    (hvv : v < 18446744073709551616) :
    ExS Gen.prog ⟨loc, m, false⟩ (.call none 5 [.var 0, offE, valE]) .normal
      ⟨loc, m.upd bo (splice blk off (encodeLE 8 v)), false⟩ := by
  have hargs : EvArgs Gen.prog ⟨loc, m, false⟩ [.var 0, offE, valE]
      [.ptr bo 0, .int IntTy.u64 off, .int IntTy.u64 v] :=
    EvArgs.cons (EvE.var h0) (EvArgs.cons hoff (EvArgs.cons hval EvArgs.nil))
  have body : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 off, .int IntTy.u64 v], m, false⟩
      Gen.tg3__wr_u64le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 off, .int IntTy.u64 v],
        m.upd bo (splice blk off (encodeLE 8 v)), false⟩ :=
    wr_u64le_writes _ m bo blk hm off v (frameLocals0 _ _ _) (frameLocals1 _ _ _)
      (frameLocals2 _ _ _) hlt hbl hvv
  exact ExS.call (f := 5) (fn := Gen.tg3__wr_u64le) hargs rfl rfl body (Or.inl rfl)

/-- Read a `u64` field of the frame object. -/
theorem evLoadU64 (loc : Locals) (m : Mem) (ro : Bool) (bf bo : Nat) (fblk : Block)
    (h1 : loc 1 = some (.ptr bf 0)) (hm : m bf = some fblk) (o v : Nat)
    (ho : o + 8 ≤ fblk.length) (hv : v < 18446744073709551616)
    (he : List.take 8 (List.drop o fblk) = encodeLE 8 v) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.load (Ty.int IntTy.u64) (.field (.var 1) o))
      (.int IntTy.u64 v) :=
  frameLoad loc m ro bf bo fblk h1 hm o IntTy.u64 8 (by rfl) ho v hv he

/-- `(uint64_t)c` for a small constant. -/
theorem evCastU64 (loc : Locals) (m : Mem) (ro : Bool) (c : Nat) (hc : c < 4294967296) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.u64 (.const (.int IntTy.u32 c)))
      (.int IntTy.u64 c) := by
  refine EvE.of_eq 5 (fun _ => ?_)
  have hc64 : c < 18446744073709551616 := by omega
  simp [evalExpr, evalCast, IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt, hc64]

/-- `f.jsonOff - c` (resp. any loaded `u64`) as an unsigned subtraction. -/
theorem evSubU64 (loc : Locals) (m : Mem) (ro : Bool) (c off : Nat) (offE valE : Expr)
    (hoff : EvE Gen.prog ⟨loc, m, ro⟩ offE (.int IntTy.u64 off))
    (hval : EvE Gen.prog ⟨loc, m, ro⟩ valE (.int IntTy.u64 c)) (hc : c ≤ off) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .sub (some IntTy.u64) offE valE) (.int IntTy.u64 (off - c)) := by
  refine EvE.bin hoff hval ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, hc]

/-- Read a `u64` field and narrow it to `u32`. -/
theorem evCastFieldU32 (loc : Locals) (m : Mem) (ro : Bool) (bf bo o v : Nat) (fblk : Block)
    (h1 : loc 1 = some (.ptr bf 0)) (hm : m bf = some fblk)
    (ho : o + 8 ≤ fblk.length) (hv : v < 18446744073709551616) (hv32 : v < 4294967296)
    (he : List.take 8 (List.drop o fblk) = encodeLE 8 v) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) o)))
      (.int IntTy.u32 v) := by
  refine EvE.cast (evLoadU64 loc m ro bf bo fblk h1 hm o v ho hv he) ?_
  simp [evalCast, IntTy.u64, IntTy.u32, IntTy.modulus, Nat.mod_eq_of_lt, hv32]

/-- Read `f.hasBin` as a truthy `i32`. -/
theorem evCondHasBin (loc : Locals) (m : Mem) (ro : Bool) (bf bo : Nat) (fblk : Block)
    (h1 : loc 1 = some (.ptr bf 0)) (hm : m bf = some fblk)
    (hlen : 44 ≤ fblk.length) (b : Bool)
    (he : List.take 4 (List.drop 36 fblk) = encodeLE 4 (b2n b)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.load (Ty.int IntTy.i32) (.field (.var 1) 36))
      (.int IntTy.i32 (b2n b)) := by
  have hfl : 36 + 4 ≤ fblk.length := by omega
  refine frameLoad loc m ro bf bo fblk h1 hm 36 IntTy.i32 4 (by rfl) hfl (b2n b) ?_ ?_
  · simp [b2n]; split <;> decide
  · exact he

/-- Read a boolean `i32` field of the frame object. -/
theorem evCondField (loc : Locals) (m : Mem) (ro : Bool) (bf bo o : Nat) (fblk : Block)
    (h1 : loc 1 = some (.ptr bf 0)) (hm : m bf = some fblk) (ho : o + 4 ≤ fblk.length) (b : Bool)
    (he : List.take 4 (List.drop o fblk) = encodeLE 4 (b2n b)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.load (Ty.int IntTy.i32) (.field (.var 1) o))
      (.int IntTy.i32 (b2n b)) := by
  refine frameLoad loc m ro bf bo fblk h1 hm o IntTy.i32 4 (by rfl) ho (b2n b) ?_ he
  simp [b2n]; split <;> decide

/-- The `tg3__glb_write_magic` call as a write of `"glTF"` at offset 0. -/
theorem write_magic_call (loc : Locals) (m : Mem) (bo : Nat) (blk : Block)
    (hm : m bo = some blk) (h0 : loc 0 = some (.ptr bo 0)) (h4 : 4 ≤ blk.length) :
    ExS Gen.prog ⟨loc, m, false⟩ (.call none 6 [.var 0]) .normal
      ⟨loc, m.upd bo (splice blk 0 [103, 108, 84, 70]), false⟩ := by
  have hl0 : frameLocals [0] [.ptr bo 0] 0 = some (.ptr bo 0) := by
    simp [frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  have hargs : EvArgs Gen.prog ⟨loc, m, false⟩ [.var 0] [.ptr bo 0] :=
    EvArgs.cons (EvE.var h0) EvArgs.nil
  have body := write_magic_writes (frameLocals [0] [.ptr bo 0]) m bo blk hm hl0 h4
  exact ExS.call (f := 6) (fn := Gen.tg3__glb_write_magic) hargs rfl rfl body (Or.inl rfl)

/-- A `u64` field minus a small constant, read through `.field 1 o`. -/
theorem evSubFieldU64 (loc : Locals) (m : Mem) (ro : Bool) (bf bo o c off : Nat) (fblk : Block)
    (h1 : loc 1 = some (.ptr bf 0)) (hm : m bf = some fblk)
    (ho : o + 8 ≤ fblk.length) (hoff : off < 18446744073709551616)
    (he : List.take 8 (List.drop o fblk) = encodeLE 8 off) (hc32 : c < 4294967296) (hc : c ≤ off) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) o))
        (.cast IntTy.u64 (.const (.int IntTy.u32 c)))) (.int IntTy.u64 (off - c)) :=
  evSubU64 loc m ro c off _ _ (evLoadU64 loc m ro bf bo fblk h1 hm o off ho hoff he)
    (evCastU64 loc m ro c hc32) hc

/-- `tg3__glb_write_header32` with has_bin = false. -/
theorem write_header32_no_bin (loc : Locals) (m : Mem) (bo bf : Nat) (blk fblk : Block)
    (f : Glb.Frame) (total : Nat)
    (hm : m bo = some blk) (hf : m bf = some fblk)
    (hFrame : FrameAt fblk f)
    (h0 : loc 0 = some (.ptr bo 0))
    (h1 : loc 1 = some (.ptr bf 0))
    (h2 : loc 2 = some (.int IntTy.u32 total))
    (hblk : blk.length < 18446744073709551616)
    (htotal : total < 4294967296)
    (hjsonOff : f.jsonOff ≥ 8)
    (hjsonLen : f.jsonLen < 4294967296)
    (hfjsonOff : f.jsonOff < 18446744073709551616)
    (hfjsonLen : f.jsonLen < 18446744073709551616)
    (hnoBin : f.hasBin = false)
    (hlen4 : 4 + 4 ≤ blk.length)
    (hlen8 : 8 + 4 ≤ blk.length)
    (hlenJ : f.jsonOff - 8 + 4 ≤ blk.length)
    (hlenJ4 : f.jsonOff - 4 + 4 ≤ blk.length)
    (hbo_ne_bf : bo ≠ bf) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_header32.body .normal
      ⟨loc, m.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)), false⟩ := by
  -- Step 1: wr_u32le(out, 4, 2)
  have hargs1 : EvArgs Gen.prog ⟨loc, m, false⟩
      [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 4)), .const (.int IntTy.u32 2)]
      [.ptr bo 0, .int IntTy.u64 4, .int IntTy.u32 2] := by
    apply EvArgs.cons
    · exact EvE.var h0
    · apply EvArgs.cons
      · refine EvE.of_eq 5 (fun _ => ?_)
        simp [evalExpr, evalCast, IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt]
      · apply EvArgs.cons
        · refine EvE.of_eq 5 (fun _ => ?_)
          simp [evalExpr]
        · exact EvArgs.nil
  have body1 : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 4, .int IntTy.u32 2], m, false⟩
      Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 4, .int IntTy.u32 2],
        m.upd bo (splice blk 4 (encodeLE 4 2)), false⟩ :=
    wr_u32le_writes (frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 4, .int IntTy.u32 2]) m bo blk hm 4 2
      (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _) (by omega) hblk (by decide)
  have step1 : ExS Gen.prog ⟨loc, m, false⟩
      (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 4)), .const (.int IntTy.u32 2)]) .normal
      ⟨loc, m.upd bo (splice blk 4 (encodeLE 4 2)), false⟩ :=
    ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs1 rfl rfl body1 (Or.inl rfl)
  -- Step 2: wr_u32le(out, 8, total)
  have hargs2 : EvArgs Gen.prog ⟨loc, m.upd bo (splice blk 4 (encodeLE 4 2)), false⟩
      [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 8)), .var 2]
      [.ptr bo 0, .int IntTy.u64 8, .int IntTy.u32 total] := by
    apply EvArgs.cons
    · exact EvE.var h0
    · apply EvArgs.cons
      · refine EvE.of_eq 5 (fun _ => ?_)
        simp [evalExpr, evalCast, IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt]
      · apply EvArgs.cons
        · exact EvE.var h2
        · exact EvArgs.nil
  have hm2 : (m.upd bo (splice blk 4 (encodeLE 4 2))) bo = some (splice blk 4 (encodeLE 4 2)) :=
    Mem.upd_same _ _ _
  have body2 : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 8, .int IntTy.u32 total],
        m.upd bo (splice blk 4 (encodeLE 4 2)), false⟩
      Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 8, .int IntTy.u32 total],
        (m.upd bo (splice blk 4 (encodeLE 4 2))).upd bo (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)), false⟩ :=
    wr_u32le_writes (frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 8, .int IntTy.u32 total])
      (m.upd bo (splice blk 4 (encodeLE 4 2))) bo (splice blk 4 (encodeLE 4 2)) hm2 8 total
      (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
      (by rw [splice_length blk 4 (encodeLE 4 2) (by rw [encodeLE_length 4 2]; exact hlen4)]; exact hlen8)
      (by rw [splice_length blk 4 (encodeLE 4 2) (by rw [encodeLE_length 4 2]; exact hlen4)]; exact hblk)
      htotal
  have step2 : ExS Gen.prog ⟨loc, m.upd bo (splice blk 4 (encodeLE 4 2)), false⟩
      (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 8)), .var 2]) .normal
      ⟨loc, (m.upd bo (splice blk 4 (encodeLE 4 2))).upd bo (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)), false⟩ :=
    ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs2 rfl rfl body2 (Or.inl rfl)
  -- Chain step1 and step2
  let m2 := (m.upd bo (splice blk 4 (encodeLE 4 2))).upd bo (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
  have chain12 : ExS Gen.prog ⟨loc, m, false⟩
      (.seq (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 4)), .const (.int IntTy.u32 2)])
        (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 8)), .var 2])) .normal
      ⟨loc, m2, false⟩ :=
    ExS.seq step1 step2
  -- Memory after step1+step2 at bo
  have hm_bo_m2 : m2 bo = some (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)) := by
    simp [m2, Mem.upd_same]
  -- bf memory unchanged in m2
  have hm_bf_m2 : m2 bf = some fblk := by
    simp [m2, Mem.upd_ne, Mem.upd_ne, hbo_ne_bf, hbo_ne_bf.symm, hf]
  -- Step 3: wr_u32le(out, json_off - 8, json_len)
  -- Load json_off
  have hev_load_joff : EvE Gen.prog ⟨loc, m2, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 1) 0)) (.int IntTy.u64 f.jsonOff) := by
    have hfl8 : 0 + 8 ≤ fblk.length := by have := hFrame.len; omega
    refine frameLoad loc m2 false bf bo fblk h1 hm_bf_m2 0 IntTy.u64 8 (by rfl) hfl8 f.jsonOff
      (by have := hfjsonOff; omega) hFrame.joff
  -- Cast 8
  have hev_cast8 : EvE Gen.prog ⟨loc, m2, false⟩
      (.cast IntTy.u64 (.const (.int IntTy.u32 8))) (.int IntTy.u64 8) := by
    refine EvE.of_eq 5 (fun _ => ?_)
    simp [evalExpr, evalCast, IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt]
  -- Bin sub json_off - 8
  have hev_sub : EvE Gen.prog ⟨loc, m2, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8)))) (.int IntTy.u64 (f.jsonOff - 8)) := by
    refine EvE.bin hev_load_joff hev_cast8 ?_
    have h : evalBin .sub (some IntTy.u64) (.int IntTy.u64 f.jsonOff) (.int IntTy.u64 8)
        = .ok (.int IntTy.u64 (f.jsonOff - 8)) := by
      have h8 : 8 ≤ f.jsonOff := hjsonOff
      simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, h8]
    exact h
  -- Load json_len
  have hev_load_jlen : EvE Gen.prog ⟨loc, m2, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 1) 8)) (.int IntTy.u64 f.jsonLen) := by
    have hfl16 : 8 + 8 ≤ fblk.length := by have := hFrame.len; omega
    refine frameLoad loc m2 false bf bo fblk h1 hm_bf_m2 8 IntTy.u64 8 (by rfl) hfl16 f.jsonLen
      (by have := hfjsonLen; omega) hFrame.jlen
  -- Cast json_len to u32
  have hev_cast_jlen : EvE Gen.prog ⟨loc, m2, false⟩
      (.cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 8))) (.int IntTy.u32 f.jsonLen) := by
    refine EvE.cast hev_load_jlen ?_
    simp [evalCast, IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt, hjsonLen]
  -- EvArgs for step3
  have hargs3 : EvArgs Gen.prog ⟨loc, m2, false⟩
      [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8))),
       .cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 8))]
      [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 8), .int IntTy.u32 f.jsonLen] := by
    apply EvArgs.cons
    · exact EvE.var h0
    · apply EvArgs.cons
      · exact hev_sub
      · apply EvArgs.cons
        · exact hev_cast_jlen
        · exact EvArgs.nil
  -- wr_u32le_writes for step3
  have hm3 : m2 bo = some (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)) := by
    simp [m2, Mem.upd_same]
  have hs1 : (splice blk 4 (encodeLE 4 2)).length = blk.length :=
    splice_length blk 4 _ (by rw [encodeLE_length]; exact hlen4)
  have hs12 : (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs1]; exact hlen8), hs1]
  have body3 : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 8), .int IntTy.u32 f.jsonLen],
        m2, false⟩
      Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 8), .int IntTy.u32 f.jsonLen],
        m2.upd bo (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)), false⟩ :=
    wr_u32le_writes (frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 8), .int IntTy.u32 f.jsonLen])
      m2 bo (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)) hm3 (f.jsonOff - 8) f.jsonLen
      (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
      (by rw [hs12]; exact hlenJ) (by rw [hs12]; exact hblk) hjsonLen
  have step3 : ExS Gen.prog ⟨loc, m2, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8))),
       .cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 8))]) .normal
      ⟨loc, m2.upd bo (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)), false⟩ :=
    ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs3 rfl rfl body3 (Or.inl rfl)
  -- The block after the first three writes.
  let m3 := m2.upd bo (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen))
  have hm_bo_m3 : m3 bo = some (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) := by simp [m3]
  have hm_bf_m3 : m3 bf = some fblk := by
    simp [m3, hm_bf_m2, hbo_ne_bf.symm]
  -- Step 4: wr_u32le(out, json_off - 4, chunkTypeJSON)
  have hev_load_joff4 : EvE Gen.prog ⟨loc, m3, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 1) 0)) (.int IntTy.u64 f.jsonOff) := by
    have hfl8 : 0 + 8 ≤ fblk.length := by have := hFrame.len; omega
    refine frameLoad loc m3 false bf bo fblk h1 hm_bf_m3 0 IntTy.u64 8 (by rfl) hfl8 f.jsonOff
      (by have := hfjsonOff; omega) hFrame.joff
  have hev_cast4 : EvE Gen.prog ⟨loc, m3, false⟩
      (.cast IntTy.u64 (.const (.int IntTy.u32 4))) (.int IntTy.u64 4) := by
    refine EvE.of_eq 5 (fun _ => ?_)
    simp [evalExpr, evalCast, IntTy.u64, IntTy.u32, IntTy.toInt, IntTy.modulus, Nat.mod_eq_of_lt]
  have hev_sub4 : EvE Gen.prog ⟨loc, m3, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4)))) (.int IntTy.u64 (f.jsonOff - 4)) := by
    refine EvE.bin hev_load_joff4 hev_cast4 ?_
    have h : evalBin .sub (some IntTy.u64) (.int IntTy.u64 f.jsonOff) (.int IntTy.u64 4)
        = .ok (.int IntTy.u64 (f.jsonOff - 4)) := by
      have h4 : 4 ≤ f.jsonOff := by omega
      simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, h4]
    exact h
  have hev_const : EvE Gen.prog ⟨loc, m3, false⟩
      (.const (.int IntTy.u32 chunkTypeJSON)) (.int IntTy.u32 chunkTypeJSON) :=
    EvE.const _
  have hargs4 : EvArgs Gen.prog ⟨loc, m3, false⟩
      [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 1313821514)]
      [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 4), .int IntTy.u32 chunkTypeJSON] := by
    apply EvArgs.cons
    · exact EvE.var h0
    · apply EvArgs.cons
      · exact hev_sub4
      · apply EvArgs.cons
        · exact hev_const
        · exact EvArgs.nil
  have hblk3 : (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs12]; exact hlenJ), hs12]
  have body4 : ExS Gen.prog
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 4), .int IntTy.u32 chunkTypeJSON],
        m3, false⟩
      Gen.tg3__wr_u32le.body .normal
      ⟨frameLocals [0, 1, 2] [.ptr bo 0, .int IntTy.u64 (f.jsonOff - 4), .int IntTy.u32 chunkTypeJSON],
        m3.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)), false⟩ :=
    wr_u32le_writes _ m3 bo _ hm_bo_m3 (f.jsonOff - 4) chunkTypeJSON
      (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
      (by rw [hblk3]; exact hlenJ4) (by rw [hblk3]; exact hblk)
      (by decide : chunkTypeJSON < 4294967296)
  have step4 : ExS Gen.prog ⟨loc, m3, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 1313821514)]) .normal
      ⟨loc, m3.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)), false⟩ :=
    ExS.call (f := 4) (fn := Gen.tg3__wr_u32le) hargs4 rfl rfl body4 (Or.inl rfl)
  -- Trailing `if (f->has_bin)` takes the `else` branch (has_bin = false).
  let m4 := m3.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
  have hm_bf_m4 : m4 bf = some fblk := by
    simp [m4, hm_bf_m3, hbo_ne_bf.symm]
  have hev_cond : EvE Gen.prog ⟨loc, m4, false⟩
      (.load (Ty.int IntTy.i32) (.field (.var 1) 36)) (.int IntTy.i32 0) := by
    have hfl36 : 36 + 4 ≤ fblk.length := by have := hFrame.len; omega
    refine frameLoad loc m4 false bf bo fblk h1 hm_bf_m4 36 IntTy.i32 4 (by rfl) hfl36 0
      (by decide) ?_
    simpa [hnoBin, b2n] using hFrame.hbn
  have hite : ExS Gen.prog ⟨loc, m4, false⟩
      (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 36))
        (.seq (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 8))),
          .cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 24))])
          (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
          .const (.int IntTy.u32 5130562)]))
        .skip) .normal ⟨loc, m4, false⟩ :=
    ExS.ite hev_cond rfl ExS.skip
  have hchain := ExS.seq step1 (ExS.seq step2 (ExS.seq step3 (ExS.seq step4 hite)))
  simp only [Gen.tg3__glb_write_header32, m2, m3, m4, Mem.upd_upd] at hchain ⊢
  exact hchain

/-- `tg3__glb_write_header32` with has_bin = true. -/
theorem write_header32_bin (loc : Locals) (m : Mem) (bo bf : Nat) (blk fblk : Block)
    (f : Glb.Frame) (total : Nat)
    (hm : m bo = some blk) (hf : m bf = some fblk)
    (hFrame : FrameAt fblk f)
    (h0 : loc 0 = some (.ptr bo 0))
    (h1 : loc 1 = some (.ptr bf 0))
    (h2 : loc 2 = some (.int IntTy.u32 total))
    (hblk : blk.length < 18446744073709551616)
    (htotal : total < 4294967296)
    (hjsonOff : f.jsonOff ≥ 8)
    (hjsonLen : f.jsonLen < 4294967296)
    (hfjsonOff : f.jsonOff < 18446744073709551616)
    (hfjsonLen : f.jsonLen < 18446744073709551616)
    (hhasBin : f.hasBin = true)
    (hbinOff : f.binOff ≥ 8)
    (hbinLen : f.binLen < 4294967296)
    (hfbinOff : f.binOff < 18446744073709551616)
    (hfbinLen : f.binLen < 18446744073709551616)
    (hlen4 : 4 + 4 ≤ blk.length)
    (hlen8 : 8 + 4 ≤ blk.length)
    (hlenJ : f.jsonOff - 8 + 4 ≤ blk.length)
    (hlenJ4 : f.jsonOff - 4 + 4 ≤ blk.length)
    (hlenB : f.binOff - 8 + 4 ≤ blk.length)
    (hlenB4 : f.binOff - 4 + 4 ≤ blk.length)
    (hbo_ne_bf : bo ≠ bf) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_header32.body .normal
      ⟨loc, m.upd bo (splice (splice (splice (splice (splice (splice blk 4 (encodeLE 4 2))
        8 (encodeLE 4 total)) (f.jsonOff - 8) (encodeLE 4 f.jsonLen))
        (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) (f.binOff - 8) (encodeLE 4 f.binLen))
        (f.binOff - 4) (encodeLE 4 chunkTypeBIN)), false⟩ := by
  have hfl8 : 0 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl16 : 8 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl24 : 24 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hflBin : 16 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl36 : 36 + 4 ≤ fblk.length := by have := hFrame.len; omega
  have hs1 : (splice blk 4 (encodeLE 4 2)).length = blk.length :=
    splice_length blk 4 _ (by rw [encodeLE_length]; exact hlen4)
  have hs2 : (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs1]; exact hlen8), hs1]
  have hs3 : (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs2]; exact hlenJ), hs2]
  have hs4 : (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)).length
      = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs3]; exact hlenJ4), hs3]
  have hs5 : (splice (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
        (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
        (f.binOff - 8) (encodeLE 4 f.binLen)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs4]; exact hlenB), hs4]
  -- Step 1
  let m1 := m.upd bo (splice blk 4 (encodeLE 4 2))
  have hm1 : m1 bo = some (splice blk 4 (encodeLE 4 2)) := by simp [m1]
  have step1 : ExS Gen.prog ⟨loc, m, false⟩
      (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 4)), .const (.int IntTy.u32 2)])
      .normal ⟨loc, m1, false⟩ :=
    wr32_call loc m bo blk hm 4 2 _ _ h0 (evCastU64 loc m false 4 (by decide)) (EvE.const _)
      hlen4 hblk (by decide)
  -- Step 2
  let m2 := m1.upd bo (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
  have hm2 : m2 bo = some (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total)) := by simp [m2]
  have hm_bf1 : m1 bf = some fblk := by simp [m1, hf, hbo_ne_bf.symm]
  have hm_bf2 : m2 bf = some fblk := by simp [m2, hm_bf1, hbo_ne_bf.symm]
  have step2 : ExS Gen.prog ⟨loc, m1, false⟩
      (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 8)), .var 2]) .normal
      ⟨loc, m2, false⟩ :=
    wr32_call loc m1 bo (splice blk 4 (encodeLE 4 2)) hm1 8 total _ _ h0
      (evCastU64 loc m1 false 8 (by decide)) (EvE.var h2)
      (by rw [hs1]; exact hlen8) (by rw [hs1]; exact hblk) htotal
  -- Step 3
  let m3 := m2.upd bo (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen))
  have hm3 : m3 bo = some (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) := by simp [m3]
  have hm_bf3 : m3 bf = some fblk := by simp [m3, hm_bf2, hbo_ne_bf.symm]
  have hoff3 : EvE Gen.prog ⟨loc, m2, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8)))) (.int IntTy.u64 (f.jsonOff - 8)) :=
    evSubFieldU64 loc m2 false bf bo 0 8 f.jsonOff fblk h1 hm_bf2 hfl8 hfjsonOff hFrame.joff
      (by decide) hjsonOff
  have hval3 : EvE Gen.prog ⟨loc, m2, false⟩
      (.cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 8))) (.int IntTy.u32 f.jsonLen) :=
    evCastFieldU32 loc m2 false bf bo 8 f.jsonLen fblk h1 hm_bf2 hfl16 hfjsonLen hjsonLen hFrame.jlen
  have step3 : ExS Gen.prog ⟨loc, m2, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8))),
       .cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 8))]) .normal
      ⟨loc, m3, false⟩ :=
    wr32_call loc m2 bo _ hm2 (f.jsonOff - 8) f.jsonLen _ _ h0 hoff3 hval3
      (by rw [hs2]; exact hlenJ) (by rw [hs2]; exact hblk) hjsonLen
  -- Step 4
  let m4 := m3.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
  have hm4 : m4 bo = some (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) := by
    simp [m4]
  have hm_bf4 : m4 bf = some fblk := by simp [m4, hm_bf3, hbo_ne_bf.symm]
  have hoff4 : EvE Gen.prog ⟨loc, m3, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4)))) (.int IntTy.u64 (f.jsonOff - 4)) :=
    evSubFieldU64 loc m3 false bf bo 0 4 f.jsonOff fblk h1 hm_bf3 hfl8 hfjsonOff hFrame.joff
      (by decide) (by omega)
  have step4 : ExS Gen.prog ⟨loc, m3, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 1313821514)]) .normal ⟨loc, m4, false⟩ :=
    wr32_call loc m3 bo _ hm3 (f.jsonOff - 4) chunkTypeJSON _ _ h0 hoff4 (EvE.const _)
      (by rw [hs3]; exact hlenJ4) (by rw [hs3]; exact hblk)
      (by decide : chunkTypeJSON < 4294967296)
  -- Bin chunk header (has_bin = true)
  have hev_cond : EvE Gen.prog ⟨loc, m4, false⟩
      (.load (Ty.int IntTy.i32) (.field (.var 1) 36)) (.int IntTy.i32 1) :=
    evCondHasBin loc m4 false bf bo fblk h1 hm_bf4 hFrame.len true (by simpa [hhasBin] using hFrame.hbn)
  let m5 := m4.upd bo (splice (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
          (f.binOff - 8) (encodeLE 4 f.binLen))
  have hm5 : m5 bo = some (splice (splice (splice (splice (splice blk 4 (encodeLE 4 2)) 8 (encodeLE 4 total))
          (f.jsonOff - 8) (encodeLE 4 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
          (f.binOff - 8) (encodeLE 4 f.binLen)) := by simp [m5]
  have hoff5 : EvE Gen.prog ⟨loc, m4, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8)))) (.int IntTy.u64 (f.binOff - 8)) :=
    evSubFieldU64 loc m4 false bf bo 16 8 f.binOff fblk h1 hm_bf4 hflBin hfbinOff hFrame.boff
      (by decide) hbinOff
  have hval5 : EvE Gen.prog ⟨loc, m4, false⟩
      (.cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 24))) (.int IntTy.u32 f.binLen) :=
    evCastFieldU32 loc m4 false bf bo 24 f.binLen fblk h1 hm_bf4 hfl24 hfbinLen hbinLen hFrame.blen
  have step5 : ExS Gen.prog ⟨loc, m4, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 8))),
       .cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 24))]) .normal
      ⟨loc, m5, false⟩ :=
    wr32_call loc m4 bo _ hm4 (f.binOff - 8) f.binLen _ _ h0 hoff5 hval5
      (by rw [hs4]; exact hlenB) (by rw [hs4]; exact hblk) hbinLen
  -- Bin chunk type
  let m6 := m5.upd bo (splice (splice (splice (splice (splice (splice blk 4 (encodeLE 4 2))
          8 (encodeLE 4 total)) (f.jsonOff - 8) (encodeLE 4 f.jsonLen))
          (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) (f.binOff - 8) (encodeLE 4 f.binLen))
          (f.binOff - 4) (encodeLE 4 chunkTypeBIN))
  have hm6 : m6 bo = some (splice (splice (splice (splice (splice (splice blk 4 (encodeLE 4 2))
          8 (encodeLE 4 total)) (f.jsonOff - 8) (encodeLE 4 f.jsonLen))
          (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) (f.binOff - 8) (encodeLE 4 f.binLen))
          (f.binOff - 4) (encodeLE 4 chunkTypeBIN)) := by simp [m6]
  have hm_bf5 : m5 bf = some fblk := by simp [m5, hm_bf4, hbo_ne_bf.symm]
  have hoff6 : EvE Gen.prog ⟨loc, m5, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4)))) (.int IntTy.u64 (f.binOff - 4)) :=
    evSubFieldU64 loc m5 false bf bo 16 4 f.binOff fblk h1 hm_bf5 hflBin hfbinOff hFrame.boff
      (by decide) (by omega)
  have step6 : ExS Gen.prog ⟨loc, m5, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 5130562)]) .normal ⟨loc, m6, false⟩ :=
    wr32_call loc m5 bo _ hm5 (f.binOff - 4) chunkTypeBIN _ _ h0 hoff6 (EvE.const _)
      (by rw [hs5]; exact hlenB4) (by rw [hs5]; exact hblk)
      (by decide : chunkTypeBIN < 4294967296)
  have hite : ExS Gen.prog ⟨loc, m4, false⟩
      (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 36))
        (.seq (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 8))),
          .cast IntTy.u32 (.load (Ty.int IntTy.u64) (.field (.var 1) 24))])
          (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
          .const (.int IntTy.u32 5130562)]))
        .skip) .normal ⟨loc, m6, false⟩ :=
    ExS.ite hev_cond rfl (ExS.seq step5 step6)
  have hchain := ExS.seq step1 (ExS.seq step2 (ExS.seq step3 (ExS.seq step4 hite)))
  simp only [Gen.tg3__glb_write_header32, m1, m2, m3, m4, m5, m6, Mem.upd_upd] at hchain ⊢
  exact hchain

/-- `tg3__glb_write_header64` with has_bin = false. -/
theorem write_header64_no_bin (loc : Locals) (m : Mem) (bo bf : Nat) (blk fblk : Block)
    (f : Glb.Frame) (total : Nat)
    (hm : m bo = some blk) (hf : m bf = some fblk)
    (hFrame : FrameAt fblk f)
    (h0 : loc 0 = some (.ptr bo 0))
    (h1 : loc 1 = some (.ptr bf 0))
    (h2 : loc 2 = some (.int IntTy.u64 total))
    (hblk : blk.length < 18446744073709551616)
    (htotal : total < 18446744073709551616)
    (hjsonOff : f.jsonOff ≥ 12)
    (hfjsonOff : f.jsonOff < 18446744073709551616)
    (hfjsonLen : f.jsonLen < 18446744073709551616)
    (hnoBin : f.hasBin = false)
    (hlen4 : 4 + 4 ≤ blk.length)
    (hlen8 : 8 + 8 ≤ blk.length)
    (hlenJ : f.jsonOff - 12 + 8 ≤ blk.length)
    (hlenJ4 : f.jsonOff - 4 + 4 ≤ blk.length)
    (hbo_ne_bf : bo ≠ bf) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_header64.body .normal
      ⟨loc, m.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
        (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)), false⟩ := by
  have hfl8 : 0 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl16 : 8 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl24 : 24 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hs1 : (splice blk 4 (encodeLE 4 3)).length = blk.length :=
    splice_length blk 4 _ (by rw [encodeLE_length]; exact hlen4)
  have hs2 : (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs1]; exact hlen8), hs1]
  have hs3 : (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
        (f.jsonOff - 12) (encodeLE 8 f.jsonLen)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs2]; exact hlenJ), hs2]
  have hs4 : (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
        (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)).length
      = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs3]; exact hlenJ4), hs3]
  -- Step 1: wr_u32le(out, 4, 3)
  let m1 := m.upd bo (splice blk 4 (encodeLE 4 3))
  have hm1 : m1 bo = some (splice blk 4 (encodeLE 4 3)) := by simp [m1]
  have step1 : ExS Gen.prog ⟨loc, m, false⟩
      (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 4)), .const (.int IntTy.u32 3)])
      .normal ⟨loc, m1, false⟩ :=
    wr32_call loc m bo blk hm 4 3 _ _ h0 (evCastU64 loc m false 4 (by decide)) (EvE.const _)
      hlen4 hblk (by decide)
  -- Step 2: wr_u64le(out, 8, total)
  let m2 := m1.upd bo (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
  have hm2 : m2 bo = some (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total)) := by simp [m2]
  have hm_bf1 : m1 bf = some fblk := by simp [m1, hf, hbo_ne_bf.symm]
  have hm_bf2 : m2 bf = some fblk := by simp [m2, hm_bf1, hbo_ne_bf.symm]
  have step2 : ExS Gen.prog ⟨loc, m1, false⟩
      (.call none 5 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 8)), .var 2]) .normal
      ⟨loc, m2, false⟩ :=
    wr64_call loc m1 bo (splice blk 4 (encodeLE 4 3)) hm1 8 total _ _ h0
      (evCastU64 loc m1 false 8 (by decide)) (EvE.var h2)
      (by rw [hs1]; exact hlen8) (by rw [hs1]; exact hblk) htotal
  -- Step 3: wr_u64le(out, json_off - 12, json_len)
  let m3 := m2.upd bo (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen))
  have hm3 : m3 bo = some (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) := by simp [m3]
  have hm_bf3 : m3 bf = some fblk := by simp [m3, hm_bf2, hbo_ne_bf.symm]
  have hoff3 : EvE Gen.prog ⟨loc, m2, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 12)))) (.int IntTy.u64 (f.jsonOff - 12)) :=
    evSubFieldU64 loc m2 false bf bo 0 12 f.jsonOff fblk h1 hm_bf2 hfl8 hfjsonOff hFrame.joff
      (by decide) hjsonOff
  have hval3 : EvE Gen.prog ⟨loc, m2, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 1) 8)) (.int IntTy.u64 f.jsonLen) :=
    evLoadU64 loc m2 false bf bo fblk h1 hm_bf2 8 f.jsonLen hfl16 hfjsonLen hFrame.jlen
  have step3 : ExS Gen.prog ⟨loc, m2, false⟩
      (.call none 5 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 12))),
       .load (Ty.int IntTy.u64) (.field (.var 1) 8)]) .normal
      ⟨loc, m3, false⟩ :=
    wr64_call loc m2 bo _ hm2 (f.jsonOff - 12) f.jsonLen _ _ h0 hoff3 hval3
      (by rw [hs2]; exact hlenJ) (by rw [hs2]; exact hblk) hfjsonLen
  -- Step 4: wr_u32le(out, json_off - 4, chunkTypeJSON)
  let m4 := m3.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
  have hm4 : m4 bo = some (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) := by
    simp [m4]
  have hm_bf4 : m4 bf = some fblk := by simp [m4, hm_bf3, hbo_ne_bf.symm]
  have hoff4 : EvE Gen.prog ⟨loc, m3, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4)))) (.int IntTy.u64 (f.jsonOff - 4)) :=
    evSubFieldU64 loc m3 false bf bo 0 4 f.jsonOff fblk h1 hm_bf3 hfl8 hfjsonOff hFrame.joff
      (by decide) (by omega)
  have step4 : ExS Gen.prog ⟨loc, m3, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 1313821514)]) .normal ⟨loc, m4, false⟩ :=
    wr32_call loc m3 bo _ hm3 (f.jsonOff - 4) chunkTypeJSON _ _ h0 hoff4 (EvE.const _)
      (by rw [hs3]; exact hlenJ4) (by rw [hs3]; exact hblk)
      (by decide : chunkTypeJSON < 4294967296)
  have hev_cond : EvE Gen.prog ⟨loc, m4, false⟩
      (.load (Ty.int IntTy.i32) (.field (.var 1) 36)) (.int IntTy.i32 0) :=
    evCondHasBin loc m4 false bf bo fblk h1 hm_bf4 hFrame.len false (by simpa [hnoBin, b2n] using hFrame.hbn)
  have hite : ExS Gen.prog ⟨loc, m4, false⟩
      (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 36))
        (.seq (.call none 5 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 12))),
          .load (Ty.int IntTy.u64) (.field (.var 1) 24)])
          (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
          .const (.int IntTy.u32 5130562)]))
        .skip) .normal ⟨loc, m4, false⟩ :=
    ExS.ite hev_cond rfl ExS.skip
  have hchain := ExS.seq step1 (ExS.seq step2 (ExS.seq step3 (ExS.seq step4 hite)))
  simp only [Gen.tg3__glb_write_header64, m1, m2, m3, m4, Mem.upd_upd] at hchain ⊢
  exact hchain

/-- `tg3__glb_write_header64` with has_bin = true. -/
theorem write_header64_bin (loc : Locals) (m : Mem) (bo bf : Nat) (blk fblk : Block)
    (f : Glb.Frame) (total : Nat)
    (hm : m bo = some blk) (hf : m bf = some fblk)
    (hFrame : FrameAt fblk f)
    (h0 : loc 0 = some (.ptr bo 0))
    (h1 : loc 1 = some (.ptr bf 0))
    (h2 : loc 2 = some (.int IntTy.u64 total))
    (hblk : blk.length < 18446744073709551616)
    (htotal : total < 18446744073709551616)
    (hjsonOff : f.jsonOff ≥ 12)
    (hfjsonOff : f.jsonOff < 18446744073709551616)
    (hfjsonLen : f.jsonLen < 18446744073709551616)
    (hhasBin : f.hasBin = true)
    (hbinOff : f.binOff ≥ 12)
    (hfbinOff : f.binOff < 18446744073709551616)
    (hfbinLen : f.binLen < 18446744073709551616)
    (hlen4 : 4 + 4 ≤ blk.length)
    (hlen8 : 8 + 8 ≤ blk.length)
    (hlenJ : f.jsonOff - 12 + 8 ≤ blk.length)
    (hlenJ4 : f.jsonOff - 4 + 4 ≤ blk.length)
    (hlenB : f.binOff - 12 + 8 ≤ blk.length)
    (hlenB4 : f.binOff - 4 + 4 ≤ blk.length)
    (hbo_ne_bf : bo ≠ bf) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_header64.body .normal
      ⟨loc, m.upd bo (splice (splice (splice (splice (splice (splice blk 4 (encodeLE 4 3))
        8 (encodeLE 8 total)) (f.jsonOff - 12) (encodeLE 8 f.jsonLen))
        (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) (f.binOff - 12) (encodeLE 8 f.binLen))
        (f.binOff - 4) (encodeLE 4 chunkTypeBIN)), false⟩ := by
  have hfl8 : 0 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl16 : 8 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hfl24 : 24 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hflBin : 16 + 8 ≤ fblk.length := by have := hFrame.len; omega
  have hs1 : (splice blk 4 (encodeLE 4 3)).length = blk.length :=
    splice_length blk 4 _ (by rw [encodeLE_length]; exact hlen4)
  have hs2 : (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs1]; exact hlen8), hs1]
  have hs3 : (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
        (f.jsonOff - 12) (encodeLE 8 f.jsonLen)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs2]; exact hlenJ), hs2]
  have hs4 : (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
        (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)).length
      = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs3]; exact hlenJ4), hs3]
  have hs5 : (splice (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
        (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
        (f.binOff - 12) (encodeLE 8 f.binLen)).length = blk.length := by
    rw [splice_length _ _ _ (by rw [encodeLE_length, hs4]; exact hlenB), hs4]
  -- Step 1
  let m1 := m.upd bo (splice blk 4 (encodeLE 4 3))
  have hm1 : m1 bo = some (splice blk 4 (encodeLE 4 3)) := by simp [m1]
  have step1 : ExS Gen.prog ⟨loc, m, false⟩
      (.call none 4 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 4)), .const (.int IntTy.u32 3)])
      .normal ⟨loc, m1, false⟩ :=
    wr32_call loc m bo blk hm 4 3 _ _ h0 (evCastU64 loc m false 4 (by decide)) (EvE.const _)
      hlen4 hblk (by decide)
  -- Step 2
  let m2 := m1.upd bo (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
  have hm2 : m2 bo = some (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total)) := by simp [m2]
  have hm_bf1 : m1 bf = some fblk := by simp [m1, hf, hbo_ne_bf.symm]
  have hm_bf2 : m2 bf = some fblk := by simp [m2, hm_bf1, hbo_ne_bf.symm]
  have step2 : ExS Gen.prog ⟨loc, m1, false⟩
      (.call none 5 [.var 0, .cast IntTy.u64 (.const (.int IntTy.u32 8)), .var 2]) .normal
      ⟨loc, m2, false⟩ :=
    wr64_call loc m1 bo (splice blk 4 (encodeLE 4 3)) hm1 8 total _ _ h0
      (evCastU64 loc m1 false 8 (by decide)) (EvE.var h2)
      (by rw [hs1]; exact hlen8) (by rw [hs1]; exact hblk) htotal
  -- Step 3
  let m3 := m2.upd bo (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen))
  have hm3 : m3 bo = some (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) := by simp [m3]
  have hm_bf3 : m3 bf = some fblk := by simp [m3, hm_bf2, hbo_ne_bf.symm]
  have hoff3 : EvE Gen.prog ⟨loc, m2, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 12)))) (.int IntTy.u64 (f.jsonOff - 12)) :=
    evSubFieldU64 loc m2 false bf bo 0 12 f.jsonOff fblk h1 hm_bf2 hfl8 hfjsonOff hFrame.joff
      (by decide) hjsonOff
  have hval3 : EvE Gen.prog ⟨loc, m2, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 1) 8)) (.int IntTy.u64 f.jsonLen) :=
    evLoadU64 loc m2 false bf bo fblk h1 hm_bf2 8 f.jsonLen hfl16 hfjsonLen hFrame.jlen
  have step3 : ExS Gen.prog ⟨loc, m2, false⟩
      (.call none 5 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 12))),
       .load (Ty.int IntTy.u64) (.field (.var 1) 8)]) .normal
      ⟨loc, m3, false⟩ :=
    wr64_call loc m2 bo _ hm2 (f.jsonOff - 12) f.jsonLen _ _ h0 hoff3 hval3
      (by rw [hs2]; exact hlenJ) (by rw [hs2]; exact hblk) hfjsonLen
  -- Step 4
  let m4 := m3.upd bo (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
  have hm4 : m4 bo = some (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) := by
    simp [m4]
  have hm_bf4 : m4 bf = some fblk := by simp [m4, hm_bf3, hbo_ne_bf.symm]
  have hoff4 : EvE Gen.prog ⟨loc, m3, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4)))) (.int IntTy.u64 (f.jsonOff - 4)) :=
    evSubFieldU64 loc m3 false bf bo 0 4 f.jsonOff fblk h1 hm_bf3 hfl8 hfjsonOff hFrame.joff
      (by decide) (by omega)
  have step4 : ExS Gen.prog ⟨loc, m3, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 0))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 1313821514)]) .normal ⟨loc, m4, false⟩ :=
    wr32_call loc m3 bo _ hm3 (f.jsonOff - 4) chunkTypeJSON _ _ h0 hoff4 (EvE.const _)
      (by rw [hs3]; exact hlenJ4) (by rw [hs3]; exact hblk)
      (by decide : chunkTypeJSON < 4294967296)
  -- Bin chunk header (has_bin = true)
  have hev_cond : EvE Gen.prog ⟨loc, m4, false⟩
      (.load (Ty.int IntTy.i32) (.field (.var 1) 36)) (.int IntTy.i32 1) :=
    evCondHasBin loc m4 false bf bo fblk h1 hm_bf4 hFrame.len true (by simpa [hhasBin] using hFrame.hbn)
  let m5 := m4.upd bo (splice (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
          (f.binOff - 12) (encodeLE 8 f.binLen))
  have hm5 : m5 bo = some (splice (splice (splice (splice (splice blk 4 (encodeLE 4 3)) 8 (encodeLE 8 total))
          (f.jsonOff - 12) (encodeLE 8 f.jsonLen)) (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON))
          (f.binOff - 12) (encodeLE 8 f.binLen)) := by simp [m5]
  have hoff5 : EvE Gen.prog ⟨loc, m4, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 12)))) (.int IntTy.u64 (f.binOff - 12)) :=
    evSubFieldU64 loc m4 false bf bo 16 12 f.binOff fblk h1 hm_bf4 hflBin hfbinOff hFrame.boff
      (by decide) hbinOff
  have hval5 : EvE Gen.prog ⟨loc, m4, false⟩
      (.load (Ty.int IntTy.u64) (.field (.var 1) 24)) (.int IntTy.u64 f.binLen) :=
    evLoadU64 loc m4 false bf bo fblk h1 hm_bf4 24 f.binLen hfl24 hfbinLen hFrame.blen
  have step5 : ExS Gen.prog ⟨loc, m4, false⟩
      (.call none 5 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 12))),
       .load (Ty.int IntTy.u64) (.field (.var 1) 24)]) .normal
      ⟨loc, m5, false⟩ :=
    wr64_call loc m4 bo _ hm4 (f.binOff - 12) f.binLen _ _ h0 hoff5 hval5
      (by rw [hs4]; exact hlenB) (by rw [hs4]; exact hblk) hfbinLen
  -- Bin chunk type
  let m6 := m5.upd bo (splice (splice (splice (splice (splice (splice blk 4 (encodeLE 4 3))
          8 (encodeLE 8 total)) (f.jsonOff - 12) (encodeLE 8 f.jsonLen))
          (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) (f.binOff - 12) (encodeLE 8 f.binLen))
          (f.binOff - 4) (encodeLE 4 chunkTypeBIN))
  have hm6 : m6 bo = some (splice (splice (splice (splice (splice (splice blk 4 (encodeLE 4 3))
          8 (encodeLE 8 total)) (f.jsonOff - 12) (encodeLE 8 f.jsonLen))
          (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)) (f.binOff - 12) (encodeLE 8 f.binLen))
          (f.binOff - 4) (encodeLE 4 chunkTypeBIN)) := by simp [m6]
  have hm_bf5 : m5 bf = some fblk := by simp [m5, hm_bf4, hbo_ne_bf.symm]
  have hoff6 : EvE Gen.prog ⟨loc, m5, false⟩
      (.bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4)))) (.int IntTy.u64 (f.binOff - 4)) :=
    evSubFieldU64 loc m5 false bf bo 16 4 f.binOff fblk h1 hm_bf5 hflBin hfbinOff hFrame.boff
      (by decide) (by omega)
  have step6 : ExS Gen.prog ⟨loc, m5, false⟩
      (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
        (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
       .const (.int IntTy.u32 5130562)]) .normal ⟨loc, m6, false⟩ :=
    wr32_call loc m5 bo _ hm5 (f.binOff - 4) chunkTypeBIN _ _ h0 hoff6 (EvE.const _)
      (by rw [hs5]; exact hlenB4) (by rw [hs5]; exact hblk)
      (by decide : chunkTypeBIN < 4294967296)
  have hite : ExS Gen.prog ⟨loc, m4, false⟩
      (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 36))
        (.seq (.call none 5 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 12))),
          .load (Ty.int IntTy.u64) (.field (.var 1) 24)])
          (.call none 4 [.var 0, .bin .sub (some IntTy.u64) (.load (Ty.int IntTy.u64) (.field (.var 1) 16))
            (.cast IntTy.u64 (.const (.int IntTy.u32 4))),
          .const (.int IntTy.u32 5130562)]))
        .skip) .normal ⟨loc, m6, false⟩ :=
    ExS.ite hev_cond rfl (ExS.seq step5 step6)
  have hchain := ExS.seq step1 (ExS.seq step2 (ExS.seq step3 (ExS.seq step4 hite)))
  simp only [Gen.tg3__glb_write_header64, m1, m2, m3, m4, m5, m6, Mem.upd_upd] at hchain ⊢
  exact hchain

/-! ## The dispatcher `tg3__glb_write_headers` -/

/-- The bytes written by the 32-bit header writer into an already-magic buffer. -/
def hdr32Bytes (f : Glb.Frame) (total : Nat) (blk : Block) : Block :=
  let b0 := splice blk 4 (encodeLE 4 2)
  let b1 := splice b0 8 (encodeLE 4 total)
  let b2 := splice b1 (f.jsonOff - 8) (encodeLE 4 f.jsonLen)
  let b3 := splice b2 (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)
  if f.hasBin then
    let b4 := splice b3 (f.binOff - 8) (encodeLE 4 f.binLen)
    splice b4 (f.binOff - 4) (encodeLE 4 chunkTypeBIN)
  else b3

/-- The bytes written by the 64-bit header writer into an already-magic buffer. -/
def hdr64Bytes (f : Glb.Frame) (total : Nat) (blk : Block) : Block :=
  let b0 := splice blk 4 (encodeLE 4 3)
  let b1 := splice b0 8 (encodeLE 8 total)
  let b2 := splice b1 (f.jsonOff - 12) (encodeLE 8 f.jsonLen)
  let b3 := splice b2 (f.jsonOff - 4) (encodeLE 4 chunkTypeJSON)
  if f.hasBin then
    let b4 := splice b3 (f.binOff - 12) (encodeLE 8 f.binLen)
    splice b4 (f.binOff - 4) (encodeLE 4 chunkTypeBIN)
  else b3

/-- `tg3__glb_write_headers` on a 32-bit frame (`is64 = false`). -/
theorem write_headers32_writes (loc : Locals) (m : Mem) (bo bf : Nat) (blk fblk : Block)
    (f : Glb.Frame) (total : Nat)
    (hm : m bo = some blk) (hf : m bf = some fblk)
    (hFrame : FrameAt fblk f)
    (h0 : loc 0 = some (.ptr bo 0))
    (h1 : loc 1 = some (.ptr bf 0))
    (h2 : loc 2 = some (.int IntTy.u64 total))
    (hblk : blk.length < 18446744073709551616)
    (htotal : total < 4294967296)
    (his64 : f.is64 = false)
    (hjsonOff : f.jsonOff ≥ 8)
    (hjsonLen : f.jsonLen < 4294967296)
    (hfjsonOff : f.jsonOff < 18446744073709551616)
    (hfjsonLen : f.jsonLen < 18446744073709551616)
    (hbinOff : f.binOff ≥ 8)
    (hbinLen : f.binLen < 4294967296)
    (hfbinOff : f.binOff < 18446744073709551616)
    (hfbinLen : f.binLen < 18446744073709551616)
    (hlen4 : 4 + 4 ≤ blk.length)
    (hlen8 : 8 + 4 ≤ blk.length)
    (hlenJ : f.jsonOff - 8 + 4 ≤ blk.length)
    (hlenJ4 : f.jsonOff - 4 + 4 ≤ blk.length)
    (hlenB : f.binOff - 8 + 4 ≤ blk.length)
    (hlenB4 : f.binOff - 4 + 4 ≤ blk.length)
    (hbo_ne_bf : bo ≠ bf) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_headers.body .normal
      ⟨loc, m.upd bo (hdr32Bytes f total (splice blk 0 [103, 108, 84, 70])), false⟩ := by
  let blk0 := splice blk 0 [103, 108, 84, 70]
  let m1 := m.upd bo blk0
  have hmagLen : blk0.length = blk.length :=
    splice_length blk 0 [103, 108, 84, 70] (by simp only [List.length_cons, List.length_nil]; omega)
  have hmagic : ExS Gen.prog ⟨loc, m, false⟩ (.call none 6 [.var 0]) .normal
      ⟨loc, m1, false⟩ :=
    write_magic_call loc m bo blk hm h0 (by omega)
  have hm1bo : m1 bo = some blk0 := by simp [m1]
  have hm1bf : m1 bf = some fblk := by simp [m1, hf, hbo_ne_bf.symm]
  have hcond : EvE Gen.prog ⟨loc, m1, false⟩
      (.load (Ty.int IntTy.i32) (.field (.var 1) 40)) (.int IntTy.i32 0) :=
    evCondField loc m1 false bf bo 40 fblk h1 hm1bf (by have := hFrame.len; omega) false
      (by simpa [his64, b2n] using hFrame.is64)
  have hcast : EvE Gen.prog ⟨loc, m1, false⟩
      (.cast IntTy.u32 (.var 2)) (.int IntTy.u32 total) := by
    refine EvE.cast (EvE.var h2) ?_
    simp [evalCast, IntTy.u64, IntTy.u32, IntTy.modulus, Nat.mod_eq_of_lt, htotal]
  have hargs : EvArgs Gen.prog ⟨loc, m1, false⟩
      [.var 0, .var 1, .cast IntTy.u32 (.var 2)]
      [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total] :=
    EvArgs.cons (EvE.var h0) (EvArgs.cons (EvE.var h1) (EvArgs.cons hcast EvArgs.nil))
  cases hbf : f.hasBin
  · have hbody : ExS Gen.prog
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total], m1, false⟩
        Gen.tg3__glb_write_header32.body .normal
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total],
          m1.upd bo (hdr32Bytes f total blk0), false⟩ := by
      have hb := write_header32_no_bin
        (frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total]) m1 bo bf blk0 fblk f total
        hm1bo hm1bf hFrame (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
        (by rw [hmagLen]; exact hblk) htotal hjsonOff hjsonLen hfjsonOff hfjsonLen hbf
        (by rw [hmagLen]; exact hlen4) (by rw [hmagLen]; exact hlen8)
        (by rw [hmagLen]; exact hlenJ) (by rw [hmagLen]; exact hlenJ4) hbo_ne_bf
      simpa [hdr32Bytes, hbf] using hb
    have stepH : ExS Gen.prog ⟨loc, m1, false⟩
        (.call none 7 [.var 0, .var 1, .cast IntTy.u32 (.var 2)]) .normal
        ⟨loc, m1.upd bo (hdr32Bytes f total blk0), false⟩ :=
      ExS.call (f := 7) (fn := Gen.tg3__glb_write_header32) hargs rfl rfl hbody (Or.inl rfl)
    have hite : ExS Gen.prog ⟨loc, m1, false⟩
        (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 40))
          (.call none 8 [.var 0, .var 1, .var 2])
          (.call none 7 [.var 0, .var 1, .cast IntTy.u32 (.var 2)])) .normal
        ⟨loc, m1.upd bo (hdr32Bytes f total blk0), false⟩ :=
      ExS.ite hcond rfl stepH
    have hchain := ExS.seq hmagic hite
    simp only [Gen.tg3__glb_write_headers, m1, blk0, hbf, hdr32Bytes, if_true, if_false, Mem.upd_upd]
      at hchain ⊢
    exact hchain
  · have hbody : ExS Gen.prog
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total], m1, false⟩
        Gen.tg3__glb_write_header32.body .normal
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total],
          m1.upd bo (hdr32Bytes f total blk0), false⟩ := by
      have hb := write_header32_bin
        (frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u32 total]) m1 bo bf blk0 fblk f total
        hm1bo hm1bf hFrame (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
        (by rw [hmagLen]; exact hblk) htotal hjsonOff hjsonLen hfjsonOff hfjsonLen hbf hbinOff hbinLen
        hfbinOff hfbinLen (by rw [hmagLen]; exact hlen4) (by rw [hmagLen]; exact hlen8)
        (by rw [hmagLen]; exact hlenJ) (by rw [hmagLen]; exact hlenJ4)
        (by rw [hmagLen]; exact hlenB) (by rw [hmagLen]; exact hlenB4) hbo_ne_bf
      simpa [hdr32Bytes, hbf] using hb
    have stepH : ExS Gen.prog ⟨loc, m1, false⟩
        (.call none 7 [.var 0, .var 1, .cast IntTy.u32 (.var 2)]) .normal
        ⟨loc, m1.upd bo (hdr32Bytes f total blk0), false⟩ :=
      ExS.call (f := 7) (fn := Gen.tg3__glb_write_header32) hargs rfl rfl hbody (Or.inl rfl)
    have hite : ExS Gen.prog ⟨loc, m1, false⟩
        (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 40))
          (.call none 8 [.var 0, .var 1, .var 2])
          (.call none 7 [.var 0, .var 1, .cast IntTy.u32 (.var 2)])) .normal
        ⟨loc, m1.upd bo (hdr32Bytes f total blk0), false⟩ :=
      ExS.ite hcond rfl stepH
    have hchain := ExS.seq hmagic hite
    simp only [Gen.tg3__glb_write_headers, m1, blk0, hbf, hdr32Bytes, if_true, if_false, Mem.upd_upd]
      at hchain ⊢
    exact hchain

/-- `tg3__glb_write_headers` on a 64-bit frame (`is64 = true`). -/
theorem write_headers64_writes (loc : Locals) (m : Mem) (bo bf : Nat) (blk fblk : Block)
    (f : Glb.Frame) (total : Nat)
    (hm : m bo = some blk) (hf : m bf = some fblk)
    (hFrame : FrameAt fblk f)
    (h0 : loc 0 = some (.ptr bo 0))
    (h1 : loc 1 = some (.ptr bf 0))
    (h2 : loc 2 = some (.int IntTy.u64 total))
    (hblk : blk.length < 18446744073709551616)
    (htotal : total < 18446744073709551616)
    (his64 : f.is64 = true)
    (hjsonOff : f.jsonOff ≥ 12)
    (hfjsonOff : f.jsonOff < 18446744073709551616)
    (hfjsonLen : f.jsonLen < 18446744073709551616)
    (hbinOff : f.binOff ≥ 12)
    (hfbinOff : f.binOff < 18446744073709551616)
    (hfbinLen : f.binLen < 18446744073709551616)
    (hlen4 : 4 + 4 ≤ blk.length)
    (hlen8 : 8 + 8 ≤ blk.length)
    (hlenJ : f.jsonOff - 12 + 8 ≤ blk.length)
    (hlenJ4 : f.jsonOff - 4 + 4 ≤ blk.length)
    (hlenB : f.binOff - 12 + 8 ≤ blk.length)
    (hlenB4 : f.binOff - 4 + 4 ≤ blk.length)
    (hbo_ne_bf : bo ≠ bf) :
    ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_write_headers.body .normal
      ⟨loc, m.upd bo (hdr64Bytes f total (splice blk 0 [103, 108, 84, 70])), false⟩ := by
  let blk0 := splice blk 0 [103, 108, 84, 70]
  let m1 := m.upd bo blk0
  have hmagLen : blk0.length = blk.length :=
    splice_length blk 0 [103, 108, 84, 70] (by simp only [List.length_cons, List.length_nil]; omega)
  have hmagic : ExS Gen.prog ⟨loc, m, false⟩ (.call none 6 [.var 0]) .normal
      ⟨loc, m1, false⟩ :=
    write_magic_call loc m bo blk hm h0 (by omega)
  have hm1bo : m1 bo = some blk0 := by simp [m1]
  have hm1bf : m1 bf = some fblk := by simp [m1, hf, hbo_ne_bf.symm]
  have hcond : EvE Gen.prog ⟨loc, m1, false⟩
      (.load (Ty.int IntTy.i32) (.field (.var 1) 40)) (.int IntTy.i32 1) :=
    evCondField loc m1 false bf bo 40 fblk h1 hm1bf (by have := hFrame.len; omega) true
      (by simpa [his64, b2n] using hFrame.is64)
  have hargs : EvArgs Gen.prog ⟨loc, m1, false⟩
      [.var 0, .var 1, .var 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total] :=
    EvArgs.cons (EvE.var h0) (EvArgs.cons (EvE.var h1) (EvArgs.cons (EvE.var h2) EvArgs.nil))
  cases hbf : f.hasBin
  · have hbody : ExS Gen.prog
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total], m1, false⟩
        Gen.tg3__glb_write_header64.body .normal
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total],
          m1.upd bo (hdr64Bytes f total blk0), false⟩ := by
      have hb := write_header64_no_bin
        (frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total]) m1 bo bf blk0 fblk f total
        hm1bo hm1bf hFrame (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
        (by rw [hmagLen]; exact hblk) htotal hjsonOff hfjsonOff hfjsonLen hbf
        (by rw [hmagLen]; exact hlen4) (by rw [hmagLen]; exact hlen8)
        (by rw [hmagLen]; exact hlenJ) (by rw [hmagLen]; exact hlenJ4) hbo_ne_bf
      simpa [hdr64Bytes, hbf] using hb
    have stepH : ExS Gen.prog ⟨loc, m1, false⟩
        (.call none 8 [.var 0, .var 1, .var 2]) .normal
        ⟨loc, m1.upd bo (hdr64Bytes f total blk0), false⟩ :=
      ExS.call (f := 8) (fn := Gen.tg3__glb_write_header64) hargs rfl rfl hbody (Or.inl rfl)
    have hite : ExS Gen.prog ⟨loc, m1, false⟩
        (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 40))
          (.call none 8 [.var 0, .var 1, .var 2])
          (.call none 7 [.var 0, .var 1, .cast IntTy.u32 (.var 2)])) .normal
        ⟨loc, m1.upd bo (hdr64Bytes f total blk0), false⟩ :=
      ExS.ite hcond rfl stepH
    have hchain := ExS.seq hmagic hite
    simp only [Gen.tg3__glb_write_headers, m1, blk0, hbf, hdr64Bytes, if_true, if_false, Mem.upd_upd]
      at hchain ⊢
    exact hchain
  · have hbody : ExS Gen.prog
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total], m1, false⟩
        Gen.tg3__glb_write_header64.body .normal
        ⟨frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total],
          m1.upd bo (hdr64Bytes f total blk0), false⟩ := by
      have hb := write_header64_bin
        (frameLocals [0, 1, 2] [.ptr bo 0, .ptr bf 0, .int IntTy.u64 total]) m1 bo bf blk0 fblk f total
        hm1bo hm1bf hFrame (frameLocals0 _ _ _) (frameLocals1 _ _ _) (frameLocals2 _ _ _)
        (by rw [hmagLen]; exact hblk) htotal hjsonOff hfjsonOff hfjsonLen hbf hbinOff hfbinOff hfbinLen
        (by rw [hmagLen]; exact hlen4) (by rw [hmagLen]; exact hlen8)
        (by rw [hmagLen]; exact hlenJ) (by rw [hmagLen]; exact hlenJ4)
        (by rw [hmagLen]; exact hlenB) (by rw [hmagLen]; exact hlenB4) hbo_ne_bf
      simpa [hdr64Bytes, hbf] using hb
    have stepH : ExS Gen.prog ⟨loc, m1, false⟩
        (.call none 8 [.var 0, .var 1, .var 2]) .normal
        ⟨loc, m1.upd bo (hdr64Bytes f total blk0), false⟩ :=
      ExS.call (f := 8) (fn := Gen.tg3__glb_write_header64) hargs rfl rfl hbody (Or.inl rfl)
    have hite : ExS Gen.prog ⟨loc, m1, false⟩
        (.ite (.load (Ty.int IntTy.i32) (.field (.var 1) 40))
          (.call none 8 [.var 0, .var 1, .var 2])
          (.call none 7 [.var 0, .var 1, .cast IntTy.u32 (.var 2)])) .normal
        ⟨loc, m1.upd bo (hdr64Bytes f total blk0), false⟩ :=
      ExS.ite hcond rfl stepH
    have hchain := ExS.seq hmagic hite
    simp only [Gen.tg3__glb_write_headers, m1, blk0, hbf, hdr64Bytes, if_true, if_false, Mem.upd_upd]
      at hchain ⊢
    exact hchain

end GlTF.C.Proofs