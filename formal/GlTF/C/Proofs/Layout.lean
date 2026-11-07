/-
  T1 (write): `tg3__glb_layout` refines `Glb.layout`.
-/
import GlTF.C.Proofs.HdrWrite
import GlTF.C.Proofs.GlbMain

namespace GlTF.C.Proofs
open GlTF GlTF.C GlTF.Glb

set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false
set_option linter.deprecated false
set_option maxRecDepth 100000
set_option maxHeartbeats 10000000

/-! ## Frame images -/

@[simp] theorem frameBytes_length (f : Glb.Frame) (t : Block) :
    (frameBytes f t).length = 44 + t.length := by
  simp [frameBytes]
  omega

@[simp] theorem take_encodeLE_eq (k n : Nat) (x : Nat) (h : n ≤ k) :
    List.take k (encodeLE n x) = encodeLE n x :=
  List.take_of_length_le (by simpa using h)

@[simp] theorem drop_encodeLE_eq (k n : Nat) (x : Nat) (h : n ≤ k) :
    List.drop k (encodeLE n x) = [] :=
  List.drop_eq_nil_of_le (by simpa using h)

theorem frameBytes_jsonOff (f : Glb.Frame) (t : Block) (v : Nat) :
    splice (frameBytes f t) 0 (encodeLE 8 v) = frameBytes { f with jsonOff := v } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

theorem frameBytes_jsonLen (f : Glb.Frame) (t : Block) (v : Nat) :
    splice (frameBytes f t) 8 (encodeLE 8 v) = frameBytes { f with jsonLen := v } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

theorem frameBytes_binOff (f : Glb.Frame) (t : Block) (v : Nat) :
    splice (frameBytes f t) 16 (encodeLE 8 v) = frameBytes { f with binOff := v } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

theorem frameBytes_binLen (f : Glb.Frame) (t : Block) (v : Nat) :
    splice (frameBytes f t) 24 (encodeLE 8 v) = frameBytes { f with binLen := v } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

theorem frameBytes_hasJson (f : Glb.Frame) (t : Block) (b : Bool) :
    splice (frameBytes f t) 32 (encodeLE 4 (b2n b)) = frameBytes { f with hasJson := b } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

theorem frameBytes_hasBin (f : Glb.Frame) (t : Block) (b : Bool) :
    splice (frameBytes f t) 36 (encodeLE 4 (b2n b)) = frameBytes { f with hasBin := b } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

theorem frameBytes_hasJson_true (f : Glb.Frame) (t : Block) :
    splice (frameBytes f t) 32 (encodeLE 4 1) = frameBytes { f with hasJson := true } t := by
  simpa [b2n] using frameBytes_hasJson f t true

theorem frameBytes_hasBin_true (f : Glb.Frame) (t : Block) :
    splice (frameBytes f t) 36 (encodeLE 4 1) = frameBytes { f with hasBin := true } t := by
  simpa [b2n] using frameBytes_hasBin f t true

theorem frameBytes_is64 (f : Glb.Frame) (t : Block) (b : Bool) :
    splice (frameBytes f t) 40 (encodeLE 4 (b2n b)) = frameBytes { f with is64 := b } t := by
  simp only [splice, frameBytes, List.take_append, List.drop_append, encodeLE_length]
  simp

/-- Splice at offset zero. -/
theorem splice_zero (blk bs : Block) : splice blk 0 bs = bs ++ blk.drop bs.length := by
  simp [splice]

theorem splice_splice_zero (blk bs1 bs2 : Block) (h : bs1.length = bs2.length) :
    splice (splice blk 0 bs1) 0 bs2 = splice blk 0 bs2 := by
  rw [splice_zero, splice_zero, ← h, List.drop_left, h, ← splice_zero]

/-- The seven initial field stores leave the zeroed frame image. -/
theorem init_bytes (fblk : Block) (b : Bool) (hlen : 44 ≤ fblk.length) :
    splice (splice (splice (splice (splice (splice (splice fblk 0 (encodeLE 8 0))
      8 (encodeLE 8 0)) 16 (encodeLE 8 0)) 24 (encodeLE 8 0)) 32 (encodeLE 4 0))
      36 (encodeLE 4 0)) 40 (encodeLE 4 (b2n b))
      = frameBytes { is64 := b } (fblk.drop 44) := by
  let X1 := encodeLE 8 0
  let X2 := encodeLE 8 0
  let X3 := encodeLE 8 0
  let X4 := encodeLE 8 0
  let X5 := encodeLE 4 0
  let X6 := encodeLE 4 0
  let X7 := encodeLE 4 (b2n b)
  have l1 : X1.length = 8 := by simp [X1]
  have l2 : X2.length = 8 := by simp [X2]
  have l3 : X3.length = 8 := by simp [X3]
  have l4 : X4.length = 8 := by simp [X4]
  have l5 : X5.length = 4 := by simp [X5]
  have l6 : X6.length = 4 := by simp [X6]
  have l7 : X7.length = 4 := by simp [X7]
  have l12 : (X1 ++ X2).length = 16 := by simp [l1, l2]
  have l123 : ((X1 ++ X2) ++ X3).length = 24 := by simp [l1, l2, l3]
  have l1234 : (((X1 ++ X2) ++ X3) ++ X4).length = 32 := by simp [l1, l2, l3, l4]
  have l12345 : ((((X1 ++ X2) ++ X3) ++ X4) ++ X5).length = 36 := by simp [l1, l2, l3, l4, l5]
  have l123456 : (((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6).length = 40 := by simp [l1, l2, l3, l4, l5, l6]
  have l1234567 : ((((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6) ++ X7).length = 44 := by simp [l1, l2, l3, l4, l5, l6, l7]
  have e1 : splice (splice fblk 0 X1) 8 X2 = splice fblk 0 (X1 ++ X2) := by
    have h := splice_append fblk 0 X1 X2 (by omega)
    rwa [show (0 : Nat) + X1.length = 8 from by omega] at h
  have e2 : splice (splice fblk 0 (X1 ++ X2)) 16 X3
      = splice fblk 0 ((X1 ++ X2) ++ X3) := by
    have h := splice_append fblk 0 (X1 ++ X2) X3 (by omega)
    rwa [show (0 : Nat) + (X1 ++ X2).length = 16 from by omega] at h
  have e3 : splice (splice fblk 0 ((X1 ++ X2) ++ X3)) 24 X4
      = splice fblk 0 (((X1 ++ X2) ++ X3) ++ X4) := by
    have h := splice_append fblk 0 ((X1 ++ X2) ++ X3) X4 (by omega)
    rwa [show (0 : Nat) + ((X1 ++ X2) ++ X3).length = 24 from by omega] at h
  have e4 : splice (splice fblk 0 (((X1 ++ X2) ++ X3) ++ X4)) 32 X5
      = splice fblk 0 ((((X1 ++ X2) ++ X3) ++ X4) ++ X5) := by
    have h := splice_append fblk 0 (((X1 ++ X2) ++ X3) ++ X4) X5 (by omega)
    rwa [show (0 : Nat) + (((X1 ++ X2) ++ X3) ++ X4).length = 32 from by omega] at h
  have e5 : splice (splice fblk 0 ((((X1 ++ X2) ++ X3) ++ X4) ++ X5)) 36 X6
      = splice fblk 0 (((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6) := by
    have h := splice_append fblk 0 ((((X1 ++ X2) ++ X3) ++ X4) ++ X5) X6 (by omega)
    rwa [show (0 : Nat) + ((((X1 ++ X2) ++ X3) ++ X4) ++ X5).length = 36
      from by omega] at h
  have e6 : splice (splice fblk 0 (((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6)) 40 X7
      = splice fblk 0 ((((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6) ++ X7) := by
    have h := splice_append fblk 0 (((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6) X7
      (by omega)
    rwa [show (0 : Nat) + (((((X1 ++ X2) ++ X3) ++ X4) ++ X5) ++ X6).length = 40
      from by omega] at h
  rw [e1, e2, e3, e4, e5, e6]
  rw [splice_zero]
  rw [l1234567]
  simp [X1, X2, X3, X4, X5, X6, X7, frameBytes, b2n, List.append_assoc]

/-! ## Expression evaluation helpers -/

/-- `(uint64_t)(int32_t)c` for a small non-negative constant. -/
theorem evCastU64I32 (loc : Locals) (m : Mem) (ro : Bool) (c : Nat) (hc : c < 2147483648) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.u64 (.const (.int IntTy.i32 c)))
      (.int IntTy.u64 c) := by
  refine EvE.cast (EvE.const _) ?_
  simp [evalCast, IntTy.i32, IntTy.u64, IntTy.toInt, IntTy.wrap, IntTy.modulus,
    Nat.mod_eq_of_lt, hc]
  omega

/-- `(uint64_t)c` for a small unsigned constant. -/
theorem evCastU64U32 (loc : Locals) (m : Mem) (ro : Bool) (c : Nat) (hc : c < 4294967296) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.u64 (.const (.int IntTy.u32 c)))
      (.int IntTy.u64 c) :=
  evCastU64 loc m ro c hc

/-- `(uint64_t)n` for a `uint64_t` constant. -/
theorem evCastU64U64 (loc : Locals) (m : Mem) (ro : Bool) (c : Nat)
    (hc : c < 18446744073709551616) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.cast IntTy.u64 (.const (.int IntTy.u64 c)))
      (.int IntTy.u64 c) := by
  refine EvE.cast (EvE.const _) ?_
  simp [evalCast, IntTy.u64, IntTy.modulus, Nat.mod_eq_of_lt hc]

/-- `is64 ? 1 : 0`, read from the `is64` parameter. -/
theorem evCondIs64 (loc : Locals) (m : Mem) (ro : Bool) (b : Bool)
    (h2 : loc 2 = some (.int IntTy.i32 (b2n b))) :
    EvE Gen.prog ⟨loc, m, ro⟩
      (.cond (.var 2) (.const (.int IntTy.i32 1)) (.const (.int IntTy.i32 0)))
      (.int IntTy.i32 (b2n b)) := by
  cases b
  · refine EvE.cond (bc := false) (EvE.var h2) (by simp [truthy, b2n]) (EvE.const _)
  · refine EvE.cond (bc := true) (EvE.var h2) (by simp [truthy, b2n]) (EvE.const _)

/-- Unsigned 64-bit addition, when it does not wrap. -/
theorem evAddU64 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u64 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u64 b))
    (h : a + b < 18446744073709551616) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .add (some IntTy.u64) aE bE) (.int IntTy.u64 (a + b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, Nat.mod_eq_of_lt h]

/-- Unsigned 64-bit subtraction, when it does not underflow. -/
theorem evSubU64Le (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u64 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u64 b))
    (h : b ≤ a) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .sub (some IntTy.u64) aE bE) (.int IntTy.u64 (a - b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, h]

/-- Unsigned 64-bit comparison. -/
theorem evGtU64 (loc : Locals) (m : Mem) (ro : Bool) (a b : Nat) (aE bE : Expr)
    (ha : EvE Gen.prog ⟨loc, m, ro⟩ aE (.int IntTy.u64 a))
    (hb : EvE Gen.prog ⟨loc, m, ro⟩ bE (.int IntTy.u64 b)) :
    EvE Gen.prog ⟨loc, m, ro⟩ (.bin .gt (some IntTy.u64) aE bE) (boolVal (a > b)) := by
  refine EvE.bin ha hb ?_
  simp [evalBin, isPtr, evalBinInt, binInt, intOf, IntTy.u64, IntTy.modulus, boolVal]

/-! ## Store and call steps -/

/-- Store a `u64` into a field of the frame object. -/
theorem store_field_u64 (loc : Locals) (m : Mem) (bf : Nat) (fblk : Block)
    (o v : Nat) (valueE : Expr)
    (h3 : loc 3 = some (.ptr bf 0)) (hm : m bf = some fblk)
    (ho : o + 8 ≤ fblk.length)
    (he : EvE Gen.prog ⟨loc, m, false⟩ valueE (.int IntTy.u64 v)) :
    ExS Gen.prog ⟨loc, m, false⟩
      (.store (.int IntTy.u64) (.field (.var 3) o) valueE) .normal
      ⟨loc, m.upd bf (splice fblk o (encodeLE 8 v)), false⟩ := by
  have haddr : EvE Gen.prog ⟨loc, m, false⟩ (.field (.var 3) o) (.ptr bf o) :=
    EvE.field (EvE.var h3) (by simp [ptrAddN, hm]; omega)
  have hst : C.store m (.int IntTy.u64) (.ptr bf o) (.int IntTy.u64 v)
      = .ok (m.upd bf (splice fblk o (encodeLE 8 v))) :=
    store_int m bf o fblk hm IntTy.u64 v (by simpa [IntTy.u64, IntTy.bytes] using ho)
  simpa using ExS.store haddr he rfl hst

/-- Store an `i32` into a field of the frame object. -/
theorem store_field_i32 (loc : Locals) (m : Mem) (bf : Nat) (fblk : Block)
    (o v : Nat) (valueE : Expr)
    (h3 : loc 3 = some (.ptr bf 0)) (hm : m bf = some fblk)
    (ho : o + 4 ≤ fblk.length)
    (he : EvE Gen.prog ⟨loc, m, false⟩ valueE (.int IntTy.i32 v)) :
    ExS Gen.prog ⟨loc, m, false⟩
      (.store (.int IntTy.i32) (.field (.var 3) o) valueE) .normal
      ⟨loc, m.upd bf (splice fblk o (encodeLE 4 v)), false⟩ := by
  have haddr : EvE Gen.prog ⟨loc, m, false⟩ (.field (.var 3) o) (.ptr bf o) :=
    EvE.field (EvE.var h3) (by simp [ptrAddN, hm]; omega)
  have hst : C.store m (.int IntTy.i32) (.ptr bf o) (.int IntTy.i32 v)
      = .ok (m.upd bf (splice fblk o (encodeLE 4 v))) :=
    store_int m bf o fblk hm IntTy.i32 v (by simpa [IntTy.i32, IntTy.bytes] using ho)
  simpa using ExS.store haddr he rfl hst

/-- Store a `u64` through the `total_out` pointer. -/
theorem store_ptr_u64 (loc : Locals) (m : Mem) (bt : Nat) (tblk : Block)
    (v : Nat) (valueE : Expr)
    (h4 : loc 4 = some (.ptr bt 0)) (hm : m bt = some tblk)
    (ho : 8 ≤ tblk.length)
    (he : EvE Gen.prog ⟨loc, m, false⟩ valueE (.int IntTy.u64 v)) :
    ExS Gen.prog ⟨loc, m, false⟩ (.store (.int IntTy.u64) (.var 4) valueE) .normal
      ⟨loc, m.upd bt (splice tblk 0 (encodeLE 8 v)), false⟩ := by
  have hst : C.store m (.int IntTy.u64) (.ptr bt 0) (.int IntTy.u64 v)
      = .ok (m.upd bt (splice tblk 0 (encodeLE 8 v))) :=
    store_int m bt 0 tblk hm IntTy.u64 v (by simpa [IntTy.u64, IntTy.bytes] using ho)
  simpa using ExS.store (EvE.var h4) he rfl hst

/-- The `jp = tg3__glb_align4(json_len)` call. -/
theorem align4_call (loc : Locals) (m : Mem) (d n : Nat) (valueE : Expr)
    (hval : EvE Gen.prog ⟨loc, m, false⟩ valueE (.int IntTy.u64 n))
    (hn : n + 3 < 18446744073709551616) :
    ExS Gen.prog ⟨loc, m, false⟩ (.call (some d) 10 [valueE]) .normal
      ⟨loc.upd d (.int IntTy.u64 (align4 n)), m, false⟩ := by
  have hargs : EvArgs Gen.prog ⟨loc, m, false⟩ [valueE] [.int IntTy.u64 n] :=
    EvArgs.cons hval EvArgs.nil
  have hl0 : frameLocals [0] [.int IntTy.u64 n] 0 = some (.int IntTy.u64 n) := by
    simp [frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  have hbody : ExS Gen.prog
      ⟨frameLocals [0] [.int IntTy.u64 n], m, false⟩ Gen.tg3__glb_align4.body
      (.ret (some (.int IntTy.u64 (align4 n))))
      ⟨frameLocals [0] [.int IntTy.u64 n], m, false⟩ :=
    align4_correct _ m false n hl0 hn
  exact ExS.call_dst (f := 10) (fn := Gen.tg3__glb_align4) hargs rfl rfl hbody

/-! ## Specification arithmetic -/

/-- The container size limit. -/
def limitOf : Bool → Nat
  | true => 18446744073709551615
  | false => 4294967295

/-- Header size before the first chunk. -/
def startOf : Bool → Nat
  | true => 16
  | false => 12

/-- Chunk-header size. -/
def hdrOf : Bool → Nat
  | true => 12
  | false => 8

theorem align4_ge (n : Nat) : n ≤ align4 n := by
  unfold align4
  have hmod : (n + 3) % 4 < 4 := Nat.mod_lt _ (by decide)
  have hdiv := Nat.div_add_mod (n + 3) 4
  omega

theorem start_hdr_le_limit (b : Bool) : startOf b + hdrOf b ≤ limitOf b := by
  cases b <;> simp [startOf, hdrOf, limitOf]

theorem three_le_limit (b : Bool) : 3 ≤ limitOf b := by
  cases b <;> simp [limitOf]

/-- If the JSON length alone overflows the alignment window, the total is too big. -/
theorem guard1_total_gt (jsonLen : Nat) (b : Bool)
    (h : jsonLen > limitOf b - 3) :
    startOf b + hdrOf b + align4 jsonLen > limitOf b := by
  have hge := align4_ge jsonLen
  have hsh : 4 ≤ startOf b + hdrOf b := by cases b <;> simp [startOf, hdrOf]
  cases b <;> simp [limitOf, startOf, hdrOf] at h ⊢ <;> omega

/-- The second JSON guard is exactly "the total exceeds the limit". -/
theorem guard2_iff (jp start hdr limit : Nat) (h : start + hdr ≤ limit) :
    (jp > limit - start - hdr) ↔ (start + hdr + jp > limit) := by
  rw [Nat.sub_sub]
  omega

/-- The guard permits computing `align4 jsonLen`. -/
theorem jsonLen_add_three_lt (jsonLen : Nat) (b : Bool)
    (h : ¬ jsonLen > limitOf b - 3) (hj : jsonLen < 18446744073709551616) :
    jsonLen + 3 < 18446744073709551616 := by
  have hle : jsonLen ≤ limitOf b - 3 := by omega
  cases b <;> simp [limitOf] at hle ⊢ <;> omega

/-- If the binary length alone overflows the alignment window, the total is too big. -/
theorem binGuard1_total_gt (total0 binLen hdr : Nat) (b : Bool)
    (h : binLen > limitOf b - 3) (hhdr : 8 ≤ hdr) :
    total0 + hdr + align4 binLen > limitOf b := by
  have hge := align4_ge binLen
  cases b <;> simp [limitOf] at h ⊢ <;> omega

/-- The binary guard is exactly "the total exceeds the limit". -/
theorem binGuard_iff (total hdr bp limit : Nat) (h : total ≤ limit) :
    (hdr > limit - total ∨ bp > limit - total - hdr) ↔ (total + hdr + bp > limit) := by
  rw [Nat.sub_sub]
  omega

/-- `Glb.layout` is `none` when the JSON total is too big. -/
theorem layout_none_of_total (jsonLen binLen : Nat) (b : Bool)
    (h : startOf b + hdrOf b + align4 jsonLen > limitOf b) :
    Glb.layout jsonLen binLen b = none := by
  cases b
  · simp [limitOf, startOf, hdrOf] at h
    simp [Glb.layout, h, limitOf, startOf, hdrOf]
  · simp [limitOf, startOf, hdrOf] at h
    simp [Glb.layout, h, limitOf, startOf, hdrOf]

/-- `Glb.layout` on an empty binary chunk. -/
theorem layout_zero_some (jsonLen : Nat) (b : Bool)
    (h : ¬ startOf b + hdrOf b + align4 jsonLen > limitOf b) :
    Glb.layout jsonLen 0 b =
      some ({ jsonOff := startOf b + hdrOf b, jsonLen := align4 jsonLen,
              hasJson := true, is64 := b },
            startOf b + hdrOf b + align4 jsonLen) := by
  cases b
  · simp [limitOf, startOf, hdrOf] at h
    simp [Glb.layout, h, limitOf, startOf, hdrOf]
  · simp [limitOf, startOf, hdrOf] at h
    simp [Glb.layout, h, limitOf, startOf, hdrOf]

/-- `Glb.layout` with a non-empty binary chunk. -/
theorem layout_bin_some (jsonLen binLen : Nat) (b : Bool)
    (hbin : binLen ≠ 0)
    (h1 : ¬ startOf b + hdrOf b + align4 jsonLen > limitOf b)
    (h2 : ¬ startOf b + hdrOf b + align4 jsonLen + hdrOf b + align4 binLen > limitOf b) :
    Glb.layout jsonLen binLen b =
      some ({ jsonOff := startOf b + hdrOf b, jsonLen := align4 jsonLen,
              binOff := startOf b + hdrOf b + align4 jsonLen + hdrOf b,
              binLen := align4 binLen, hasJson := true, hasBin := true, is64 := b },
            startOf b + hdrOf b + align4 jsonLen + hdrOf b + align4 binLen) := by
  cases b
  · simp [limitOf, startOf, hdrOf] at hbin h1 h2
    simp [Glb.layout, hbin, h1, h2, limitOf, startOf, hdrOf]
  · simp [limitOf, startOf, hdrOf] at hbin h1 h2
    simp [Glb.layout, hbin, h1, h2, limitOf, startOf, hdrOf]

/-- `Glb.layout` is `none` when the binary total is too big. -/
theorem layout_none_of_bin_total (jsonLen binLen : Nat) (b : Bool)
    (hbin : binLen ≠ 0)
    (h1 : ¬ startOf b + hdrOf b + align4 jsonLen > limitOf b)
    (h2 : startOf b + hdrOf b + align4 jsonLen + hdrOf b + align4 binLen > limitOf b) :
    Glb.layout jsonLen binLen b = none := by
  cases b
  · simp [limitOf, startOf, hdrOf] at hbin h1 h2
    simp [Glb.layout, hbin, h1, h2, limitOf, startOf, hdrOf]
  · simp [limitOf, startOf, hdrOf] at hbin h1 h2
    simp [Glb.layout, hbin, h1, h2, limitOf, startOf, hdrOf]

theorem start_hdr_lt_two64 (b : Bool) : startOf b + hdrOf b < 18446744073709551616 := by
  cases b <;> simp [startOf, hdrOf]

theorem limit_lt_two64 (b : Bool) : limitOf b < 18446744073709551616 := by
  cases b <;> simp [limitOf]

theorem hdr_ge_eight (b : Bool) : 8 ≤ hdrOf b := by
  cases b <;> simp [hdrOf]


/-! ## The translated statement, split into named pieces -/

def eCast0 : Expr := .cast IntTy.u64 (.const (.int IntTy.i32 0))
def eCast0u : Expr := .cast IntTy.u64 (.const (.int IntTy.u32 0))
def eCast16 : Expr := .cast IntTy.u64 (.const (.int IntTy.u32 16))
def eCast12 : Expr := .cast IntTy.u64 (.const (.int IntTy.u32 12))
def eCast8 : Expr := .cast IntTy.u64 (.const (.int IntTy.u32 8))
def eCast3 : Expr := .cast IntTy.u64 (.const (.int IntTy.u32 3))
def eLim64 : Expr := .cast IntTy.u64 (.const (.int IntTy.u64 18446744073709551615))
def eLim32 : Expr := .cast IntTy.u64 (.const (.int IntTy.u64 4294967295))
def eStartAddHdr : Expr := .bin .add (some IntTy.u64) (.var 6) (.var 7)
def eStartHdrJp : Expr := .bin .add (some IntTy.u64) (.bin .add (some IntTy.u64) (.var 6) (.var 7)) (.var 8)
def eLimMinus3 : Expr := .bin .sub (some IntTy.u64) (.var 5) eCast3
def eJpSub : Expr := .bin .sub (some IntTy.u64)
  (.bin .sub (some IntTy.u64) (.var 5) (.var 6)) (.var 7)
def eJpGuard : Expr := .bin .gt (some IntTy.u64) (.var 8) eJpSub
def eBinCond : Expr := .bin .gt (some IntTy.u64) (.var 1) eCast0u
def eBinGuard1 : Expr := .bin .gt (some IntTy.u64) (.var 1) eLimMinus3
def eGuard2a : Expr := .bin .gt (some IntTy.u64) (.var 7)
  (.bin .sub (some IntTy.u64) (.var 5) (.var 10))
def eGuard2b : Expr := .bin .gt (some IntTy.u64) (.var 9)
  (.bin .sub (some IntTy.u64) (.bin .sub (some IntTy.u64) (.var 5) (.var 10)) (.var 7))
def eBinGuard2 : Expr := .lor eGuard2a eGuard2b
def eTotalHdr : Expr := .bin .add (some IntTy.u64) (.var 10) (.var 7)
def eTotalHdrBp : Expr := .bin .add (some IntTy.u64) (.bin .add (some IntTy.u64) (.var 10) (.var 7)) (.var 9)

def s0 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 0) eCast0
def s1 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 8) eCast0
def s2 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 16) eCast0
def s3 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 24) eCast0
def s4 : Stmt := .store (.int IntTy.i32) (.field (.var 3) 32) (.const (.int IntTy.i32 0))
def s5 : Stmt := .store (.int IntTy.i32) (.field (.var 3) 36) (.const (.int IntTy.i32 0))
def s6 : Stmt := .store (.int IntTy.i32) (.field (.var 3) 40)
  (.cond (.var 2) (.const (.int IntTy.i32 1)) (.const (.int IntTy.i32 0)))
def s7 : Stmt := .store (.int IntTy.u64) (.var 4) eCast0
def s8 : Stmt := .ite (.var 2)
  (.seq (.set 5 eLim64) (.seq (.set 6 eCast16) (.set 7 eCast12)))
  (.seq (.set 5 eLim32) (.seq (.set 6 eCast12) (.set 7 eCast8)))

def ret0 : Stmt := .ret (some (.const (.int IntTy.i32 0)))
def ret1 : Stmt := .ret (some (.const (.int IntTy.i32 1)))

def jsonGuard : Stmt := .ite
  (.bin .gt (some IntTy.u64) (.var 0) eLimMinus3) ret0 .skip

def s10 : Stmt := .call (some 8) 10 [(.var 0)]
def jpGuard : Stmt := .ite eJpGuard ret0 .skip
def s12 : Stmt := .set 10 eStartHdrJp
def s13 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 0) eStartAddHdr
def s14 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 8) (.var 8)
def s15 : Stmt := .store (.int IntTy.i32) (.field (.var 3) 32) (.const (.int IntTy.i32 1))

def binGuard1 : Stmt := .ite eBinGuard1 ret0 .skip
def bCall : Stmt := .call (some 9) 10 [(.var 1)]
def binGuard2 : Stmt := .ite eBinGuard2 ret0 .skip
def b12 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 16) eTotalHdr
def b13 : Stmt := .store (.int IntTy.u64) (.field (.var 3) 24) (.var 9)
def b14 : Stmt := .store (.int IntTy.i32) (.field (.var 3) 36) (.const (.int IntTy.i32 1))
def b15 : Stmt := .set 10 eTotalHdrBp
def binBody : Stmt := .seq binGuard1 (.seq bCall (.seq binGuard2 (.seq b12 (.seq b13 (.seq b14 b15)))))
def binCond : Stmt := .ite eBinCond binBody .skip

def s17 : Stmt := .store (.int IntTy.u64) (.var 4) (.var 10)

def layoutRest : Stmt :=
  .seq jsonGuard (.seq s10 (.seq jpGuard (.seq s12 (.seq s13 (.seq s14 (.seq s15
    (.seq binCond (.seq s17 ret1))))))))


theorem rest9_eq : dropSeq 9 Gen.tg3__glb_layout.body = layoutRest := rfl

theorem body_structure : Gen.tg3__glb_layout.body =
    .seq s0 (.seq s1 (.seq s2 (.seq s3 (.seq s4 (.seq s5 (.seq s6 (.seq s7
      (.seq s8 layoutRest)))))))) := rfl

/-! ## Refinement of `layoutRest` -/

theorem layout_rest (loc : Locals) (m : Mem) (jsonLen binLen : Nat) (b : Bool)
    (bf bt : Nat) (fblk tblk : Block)
    (h0 : loc 0 = some (.int IntTy.u64 jsonLen))
    (h1 : loc 1 = some (.int IntTy.u64 binLen))
    (h3 : loc 3 = some (.ptr bf 0))
    (h4 : loc 4 = some (.ptr bt 0))
    (h5 : loc 5 = some (.int IntTy.u64 (limitOf b)))
    (h6 : loc 6 = some (.int IntTy.u64 (startOf b)))
    (h7 : loc 7 = some (.int IntTy.u64 (hdrOf b)))
    (hmf : m bf = some (frameBytes { is64 := b } (fblk.drop 44)))
    (hmt : m bt = some tblk)
    (hlenf : 44 ≤ fblk.length) (hlent : 8 ≤ tblk.length) (hne : bf ≠ bt)
    (hj : jsonLen < 18446744073709551616) (hbl : binLen < 18446744073709551616) :
    ∃ s', ExS Gen.prog ⟨loc, m, false⟩ layoutRest
        (.ret (some (.int IntTy.i32
          (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0)))) s' ∧
      (∀ fr total, Glb.layout jsonLen binLen b = some (fr, total) →
        s'.mem bf = some (frameBytes fr (fblk.drop 44)) ∧
        s'.mem bt = some (splice tblk 0 (encodeLE 8 total))) := by
  have hstart : startOf b + hdrOf b ≤ limitOf b := start_hdr_le_limit b
  -- JSON guard
  have hcond1 : EvE Gen.prog ⟨loc, m, false⟩
      (.bin .gt (some IntTy.u64) (.var 0) eLimMinus3) (boolVal (jsonLen > limitOf b - 3)) := by
    refine evGtU64 loc m false jsonLen (limitOf b - 3) (.var 0) eLimMinus3
      (EvE.var h0) ?_
    exact evSubU64Le loc m false (limitOf b) 3 (.var 5) eCast3
      (EvE.var h5) (evCastU64U32 loc m false 3 (by decide)) (three_le_limit b)
  by_cases hg1 : jsonLen > limitOf b - 3
  · -- first guard fires: return 0
    have hgt := guard1_total_gt jsonLen b hg1
    have hlay : Glb.layout jsonLen binLen b = none := layout_none_of_total _ _ _ hgt
    have hcode : (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0) = 0 := by
      simp [hlay]
    have hite : ExS Gen.prog ⟨loc, m, false⟩ jsonGuard
        (.ret (some (.int IntTy.i32 0))) ⟨loc, m, false⟩ :=
      ExS.ite (bc := true) hcond1 (by simp [truthy, boolVal, hg1]) (ExS.ret (EvE.const _))
    refine ⟨⟨loc, m, false⟩, ?_, ?_⟩
    · rw [hcode]
      simp only [layoutRest]
      exact ExS.seq_ret hite
    · intro fr total h; rw [hlay] at h; cases h
  · -- JSON length fits; align it
    have hj3 : jsonLen + 3 < 18446744073709551616 := jsonLen_add_three_lt jsonLen b hg1 hj
    have hite : ExS Gen.prog ⟨loc, m, false⟩ jsonGuard .normal ⟨loc, m, false⟩ :=
      ExS.ite (bc := false) hcond1 (by simp [truthy, boolVal, hg1]) ExS.skip
    have hcall : ExS Gen.prog ⟨loc, m, false⟩ s10 .normal
        ⟨loc.upd 8 (.int IntTy.u64 (align4 jsonLen)), m, false⟩ :=
      align4_call loc m 8 jsonLen (.var 0) (EvE.var h0) hj3
    let l1 : Locals := loc.upd 8 (.int IntTy.u64 (align4 jsonLen))
    have hsub1 : EvE Gen.prog ⟨l1, m, false⟩
        (.bin .sub (some IntTy.u64) (.var 5) (.var 6))
        (.int IntTy.u64 (limitOf b - startOf b)) :=
      evSubU64Le l1 m false (limitOf b) (startOf b) (.var 5) (.var 6)
        (EvE.var (by simp [l1, h5])) (EvE.var (by simp [l1, h6])) (by omega)
    have hsub2 : EvE Gen.prog ⟨l1, m, false⟩ eJpSub
        (.int IntTy.u64 ((limitOf b - startOf b) - hdrOf b)) := by
      refine evSubU64Le l1 m false (limitOf b - startOf b) (hdrOf b)
        (.bin .sub (some IntTy.u64) (.var 5) (.var 6)) (.var 7) hsub1
        (EvE.var (by simp [l1, h7])) ?_
      omega
    have hcond2 : EvE Gen.prog ⟨l1, m, false⟩ eJpGuard
        (boolVal (align4 jsonLen > (limitOf b - startOf b) - hdrOf b)) :=
      evGtU64 l1 m false (align4 jsonLen) ((limitOf b - startOf b) - hdrOf b)
        (.var 8) eJpSub (EvE.var (by simp [l1])) hsub2
    by_cases hg2 : align4 jsonLen > (limitOf b - startOf b) - hdrOf b
    · -- second guard fires: return 0
      have hgt : startOf b + hdrOf b + align4 jsonLen > limitOf b :=
        (guard2_iff (align4 jsonLen) (startOf b) (hdrOf b) (limitOf b) hstart).mp hg2
      have hlay : Glb.layout jsonLen binLen b = none := layout_none_of_total _ _ _ hgt
      have hcode : (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0) = 0 := by
        simp [hlay]
      have hite2 : ExS Gen.prog ⟨l1, m, false⟩ jpGuard
          (.ret (some (.int IntTy.i32 0))) ⟨l1, m, false⟩ :=
        ExS.ite (bc := true) hcond2 (by simp [truthy, boolVal, hg2]) (ExS.ret (EvE.const _))
      refine ⟨⟨l1, m, false⟩, ?_, ?_⟩
      · rw [hcode]
        simp only [layoutRest]
        exact ExS.seq hite (ExS.seq hcall (ExS.seq_ret hite2))
      · intro fr total h; rw [hlay] at h; cases h
    · -- total fits
      have htotal_le : ¬ startOf b + hdrOf b + align4 jsonLen > limitOf b := by
        intro h
        exact hg2 ((guard2_iff (align4 jsonLen) (startOf b) (hdrOf b) (limitOf b) hstart).mpr h)
      have htotal_lt : startOf b + hdrOf b + align4 jsonLen < 18446744073709551616 := by
        have := limit_lt_two64 b; omega
      have hite2 : ExS Gen.prog ⟨l1, m, false⟩ jpGuard .normal ⟨l1, m, false⟩ :=
        ExS.ite (bc := false) hcond2 (by simp [truthy, boolVal, hg2]) ExS.skip
      -- total = start + hdr + jp
      have hset : ExS Gen.prog ⟨l1, m, false⟩ s12 .normal
          ⟨l1.upd 10 (.int IntTy.u64 (startOf b + hdrOf b + align4 jsonLen)), m, false⟩ := by
        refine ExS.set ?_
        refine evAddU64 l1 m false (startOf b + hdrOf b) (align4 jsonLen)
          (.bin .add (some IntTy.u64) (.var 6) (.var 7)) (.var 8) ?_ (EvE.var (by simp [l1])) ?_
        · refine evAddU64 l1 m false (startOf b) (hdrOf b) (.var 6) (.var 7)
            (EvE.var (by simp [l1, h6])) (EvE.var (by simp [l1, h7])) ?_
          exact start_hdr_lt_two64 b
        · exact htotal_lt
      let l2 : Locals := l1.upd 10 (.int IntTy.u64 (startOf b + hdrOf b + align4 jsonLen))
      let f1 : Glb.Frame := { jsonOff := startOf b + hdrOf b, is64 := b }
      let f2 : Glb.Frame := { f1 with jsonLen := align4 jsonLen }
      let f3 : Glb.Frame := { f2 with hasJson := true }
      -- store json_off
      have hstore1 : ExS Gen.prog ⟨l2, m, false⟩ s13 .normal
          ⟨l2, m.upd bf (frameBytes f1 (fblk.drop 44)), false⟩ := by
        have ho : 0 + 8 ≤ (frameBytes { is64 := b } (fblk.drop 44)).length := by
          rw [frameBytes_length]; omega
        have he : EvE Gen.prog ⟨l2, m, false⟩ eStartAddHdr
            (.int IntTy.u64 (startOf b + hdrOf b)) := by
          refine evAddU64 l2 m false (startOf b) (hdrOf b) (.var 6) (.var 7)
            (EvE.var (by simp [l2, l1, h6])) (EvE.var (by simp [l2, l1, h7])) ?_
          exact start_hdr_lt_two64 b
        have hs := store_field_u64 l2 m bf (frameBytes { is64 := b } (fblk.drop 44))
          0 (startOf b + hdrOf b) eStartAddHdr (by simp [l2, l1, h3]) hmf ho he
        simpa [s13, f1, frameBytes_jsonOff] using hs
      -- store json_len
      have hstore2 : ExS Gen.prog ⟨l2, m.upd bf (frameBytes f1 (fblk.drop 44)), false⟩ s14 .normal
          ⟨l2, m.upd bf (frameBytes f2 (fblk.drop 44)), false⟩ := by
        have hm' : (m.upd bf (frameBytes f1 (fblk.drop 44))) bf
            = some (frameBytes f1 (fblk.drop 44)) := by simp
        have ho : 8 + 8 ≤ (frameBytes f1 (fblk.drop 44)).length := by
          rw [frameBytes_length]; omega
        have he : EvE Gen.prog ⟨l2, m.upd bf (frameBytes f1 (fblk.drop 44)), false⟩ (.var 8)
            (.int IntTy.u64 (align4 jsonLen)) := EvE.var (by simp [l2, l1])
        have hs := store_field_u64 l2 (m.upd bf (frameBytes f1 (fblk.drop 44))) bf
          (frameBytes f1 (fblk.drop 44)) 8 (align4 jsonLen) (.var 8)
          (by simp [l2, l1, h3]) hm' ho he
        simpa [s14, f2, frameBytes_jsonLen] using hs
      -- store has_json
      have hstore3 : ExS Gen.prog ⟨l2, m.upd bf (frameBytes f2 (fblk.drop 44)), false⟩ s15 .normal
          ⟨l2, m.upd bf (frameBytes f3 (fblk.drop 44)), false⟩ := by
        have hm' : (m.upd bf (frameBytes f2 (fblk.drop 44))) bf
            = some (frameBytes f2 (fblk.drop 44)) := by simp
        have ho : 32 + 4 ≤ (frameBytes f2 (fblk.drop 44)).length := by
          rw [frameBytes_length]; omega
        have he : EvE Gen.prog ⟨l2, m.upd bf (frameBytes f2 (fblk.drop 44)), false⟩
            (.const (.int IntTy.i32 1)) (.int IntTy.i32 1) := EvE.const _
        have hs := store_field_i32 l2 (m.upd bf (frameBytes f2 (fblk.drop 44))) bf
          (frameBytes f2 (fblk.drop 44)) 32 1 (.const (.int IntTy.i32 1))
          (by simp [l2, l1, h3]) hm' ho he
        simpa [s15, f3, frameBytes_hasJson_true] using hs
      let mF3 : Mem := m.upd bf (frameBytes f3 (fblk.drop 44))
      let t0 : Nat := startOf b + hdrOf b + align4 jsonLen
      have htle : t0 ≤ limitOf b := by omega
      have hcondBin : EvE Gen.prog ⟨l2, mF3, false⟩ eBinCond (boolVal (binLen > 0)) := by
        refine evGtU64 l2 mF3 false binLen 0 (.var 1) eCast0u
          (EvE.var (by simp [mF3, l2, l1, h1])) ?_
        exact evCastU64U32 l2 mF3 false 0 (by decide)
      by_cases hbin : binLen > 0
      · -- binary chunk present
        have hbin_ne : binLen ≠ 0 := by omega
        have hcondBin1 : EvE Gen.prog ⟨l2, mF3, false⟩ eBinGuard1
            (boolVal (binLen > limitOf b - 3)) := by
          refine evGtU64 l2 mF3 false binLen (limitOf b - 3) (.var 1) eLimMinus3
            (EvE.var (by simp [mF3, l2, l1, h1])) ?_
          exact evSubU64Le l2 mF3 false (limitOf b) 3 (.var 5) eCast3
            (EvE.var (by simp [mF3, l2, l1, h5]))
            (evCastU64U32 l2 mF3 false 3 (by decide)) (three_le_limit b)
        by_cases hbg1 : binLen > limitOf b - 3
        · -- binary guard fires: return 0
          have hgt := binGuard1_total_gt t0 binLen (hdrOf b) b hbg1 (hdr_ge_eight b)
          have hlay := layout_none_of_bin_total jsonLen binLen b hbin_ne htotal_le hgt
          have hcode : (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0) = 0 := by
            simp [hlay]
          have hg1ite : ExS Gen.prog ⟨l2, mF3, false⟩ binGuard1
              (.ret (some (.int IntTy.i32 0))) ⟨l2, mF3, false⟩ :=
            ExS.ite (bc := true) hcondBin1 (by simp [truthy, boolVal, hbg1])
              (ExS.ret (EvE.const _))
          have hbinBody : ExS Gen.prog ⟨l2, mF3, false⟩ binBody
              (.ret (some (.int IntTy.i32 0))) ⟨l2, mF3, false⟩ :=
            ExS.seq_ret hg1ite
          have hbinIte : ExS Gen.prog ⟨l2, mF3, false⟩ binCond
              (.ret (some (.int IntTy.i32 0))) ⟨l2, mF3, false⟩ :=
            ExS.ite (bc := true) hcondBin (by simp [truthy, boolVal, hbin]) hbinBody
          refine ⟨⟨l2, mF3, false⟩, ?_, ?_⟩
          · rw [hcode]
            simp only [layoutRest]
            have hbinTail : ExS Gen.prog ⟨l2, mF3, false⟩ (.seq binCond (.seq s17 ret1))
                (.ret (some (.int IntTy.i32 0))) ⟨l2, mF3, false⟩ := ExS.seq_ret hbinIte
            exact ExS.seq hite (ExS.seq hcall (ExS.seq hite2 (ExS.seq hset
              (ExS.seq hstore1 (ExS.seq hstore2 (ExS.seq hstore3 hbinTail))))))
          · intro fr total h; rw [hlay] at h; cases h
        · -- align the binary chunk
          have hbin3 : binLen + 3 < 18446744073709551616 := jsonLen_add_three_lt binLen b hbg1 hbl
          have hguard1Ite : ExS Gen.prog ⟨l2, mF3, false⟩ binGuard1 .normal ⟨l2, mF3, false⟩ :=
            ExS.ite (bc := false) hcondBin1 (by simp [truthy, boolVal, hbg1]) ExS.skip
          have hcallB : ExS Gen.prog ⟨l2, mF3, false⟩ (.call (some 9) 10 [(.var 1)])
              .normal ⟨l2.upd 9 (.int IntTy.u64 (align4 binLen)), mF3, false⟩ :=
            align4_call l2 mF3 9 binLen (.var 1)
              (EvE.var (by simp [mF3, l2, l1, h1])) hbin3
          let l3 : Locals := l2.upd 9 (.int IntTy.u64 (align4 binLen))
          have hsubT : EvE Gen.prog ⟨l3, mF3, false⟩
              (.bin .sub (some IntTy.u64) (.var 5) (.var 10))
              (.int IntTy.u64 (limitOf b - t0)) :=
            evSubU64Le l3 mF3 false (limitOf b) t0 (.var 5) (.var 10)
              (EvE.var (by simp [l3, l2, l1, h5]))
              (EvE.var (by simp [l3, l2, l1, t0])) (by omega)
          have hgtT : EvE Gen.prog ⟨l3, mF3, false⟩
              (.bin .gt (some IntTy.u64) (.var 7) (.bin .sub (some IntTy.u64) (.var 5) (.var 10)))
              (boolVal (hdrOf b > limitOf b - t0)) :=
            evGtU64 l3 mF3 false (hdrOf b) (limitOf b - t0) (.var 7)
              (.bin .sub (some IntTy.u64) (.var 5) (.var 10))
              (EvE.var (by simp [l3, l2, l1, h7])) hsubT
          have hcondBin2 : EvE Gen.prog ⟨l3, mF3, false⟩ eBinGuard2
              (boolVal (decide (hdrOf b > limitOf b - t0) || decide (align4 binLen > (limitOf b - t0) - hdrOf b))) := by
            change EvE Gen.prog ⟨l3, mF3, false⟩ (.lor eGuard2a eGuard2b)
              (boolVal (decide (hdrOf b > limitOf b - t0) || decide (align4 binLen > (limitOf b - t0) - hdrOf b)))
            by_cases hg2a : hdrOf b > limitOf b - t0
            · have h := EvE.lor_true (a := eGuard2a) (b := eGuard2b) hgtT (by simp [truthy, boolVal, hg2a])
              simpa [hg2a, Bool.true_or, decide_eq_true_eq] using h
            · have hsubT2 : EvE Gen.prog ⟨l3, mF3, false⟩
                  (.bin .sub (some IntTy.u64) (.bin .sub (some IntTy.u64) (.var 5) (.var 10)) (.var 7))
                  (.int IntTy.u64 ((limitOf b - t0) - hdrOf b)) :=
                evSubU64Le l3 mF3 false (limitOf b - t0) (hdrOf b)
                  (.bin .sub (some IntTy.u64) (.var 5) (.var 10)) (.var 7) hsubT
                  (EvE.var (by simp [l3, l2, l1, h7])) (by omega)
              have hgtB : EvE Gen.prog ⟨l3, mF3, false⟩
                  (.bin .gt (some IntTy.u64) (.var 9)
                    (.bin .sub (some IntTy.u64)
                      (.bin .sub (some IntTy.u64) (.var 5) (.var 10)) (.var 7)))
                  (boolVal (align4 binLen > (limitOf b - t0) - hdrOf b)) :=
                evGtU64 l3 mF3 false (align4 binLen) ((limitOf b - t0) - hdrOf b)
                  (.var 9)
                  (.bin .sub (some IntTy.u64)
                    (.bin .sub (some IntTy.u64) (.var 5) (.var 10)) (.var 7))
                  (EvE.var (by simp [l3, l2, l1, t0])) hsubT2
              have hb : truthy (boolVal (decide (align4 binLen > (limitOf b - t0) - hdrOf b)))
                  = .ok (decide (hdrOf b > limitOf b - t0) || decide (align4 binLen > (limitOf b - t0) - hdrOf b)) := by
                simp only [truthy, boolVal, hg2a, Bool.false_or]
                by_cases hc : (limitOf b - t0 - hdrOf b) < align4 binLen <;>
                  simp [hc, decide_eq_true_eq]
              exact EvE.lor_false (a := eGuard2a) (b := eGuard2b) hgtT (by simp [truthy, boolVal, hg2a]) hgtB hb
          have hg2iff := binGuard_iff t0 (hdrOf b) (align4 binLen) (limitOf b) htle
          by_cases hbg2 : hdrOf b > limitOf b - t0 ∨ align4 binLen > (limitOf b - t0) - hdrOf b
          · -- binary guard fires: return 0
            have hgt : t0 + hdrOf b + align4 binLen > limitOf b := hg2iff.mp hbg2
            have hlay := layout_none_of_bin_total jsonLen binLen b hbin_ne htotal_le hgt
            have hcode : (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0) = 0 := by
              simp [hlay]
            have hguard2Ite : ExS Gen.prog ⟨l3, mF3, false⟩ binGuard2
                (.ret (some (.int IntTy.i32 0))) ⟨l3, mF3, false⟩ :=
              ExS.ite (bc := true) hcondBin2 (by simp [truthy, boolVal, hbg2])
                (ExS.ret (EvE.const _))
            have hbinBody : ExS Gen.prog ⟨l2, mF3, false⟩ binBody
                (.ret (some (.int IntTy.i32 0))) ⟨l3, mF3, false⟩ :=
              ExS.seq hguard1Ite (ExS.seq hcallB (ExS.seq_ret hguard2Ite))
            have hbinIte : ExS Gen.prog ⟨l2, mF3, false⟩ binCond
                (.ret (some (.int IntTy.i32 0))) ⟨l3, mF3, false⟩ :=
              ExS.ite (bc := true) hcondBin (by simp [truthy, boolVal, hbin]) hbinBody
            refine ⟨⟨l3, mF3, false⟩, ?_, ?_⟩
            · rw [hcode]
              simp only [layoutRest]
              have hbinTail : ExS Gen.prog ⟨l2, mF3, false⟩ (.seq binCond (.seq s17 ret1))
                  (.ret (some (.int IntTy.i32 0))) ⟨l3, mF3, false⟩ := ExS.seq_ret hbinIte
              exact ExS.seq hite (ExS.seq hcall (ExS.seq hite2 (ExS.seq hset
                (ExS.seq hstore1 (ExS.seq hstore2 (ExS.seq hstore3 hbinTail))))))
            · intro fr total h; rw [hlay] at h; cases h
          · -- binary chunk fits
            have htb_le : ¬ t0 + hdrOf b + align4 binLen > limitOf b := by
              intro h; exact hbg2 (hg2iff.mpr h)
            let f4 : Glb.Frame := { f3 with binOff := t0 + hdrOf b }
            let f5 : Glb.Frame := { f4 with binLen := align4 binLen }
            let f6 : Glb.Frame := { f5 with hasBin := true }
            let l4 : Locals := l3.upd 10 (.int IntTy.u64 (t0 + hdrOf b + align4 binLen))
            have hguard2Ite : ExS Gen.prog ⟨l3, mF3, false⟩ binGuard2 .normal ⟨l3, mF3, false⟩ :=
              ExS.ite (bc := false) hcondBin2 (by simp [truthy, boolVal, hbg2]) ExS.skip
            -- store bin_off
            have hb12 : ExS Gen.prog ⟨l3, mF3, false⟩ b12 .normal
                ⟨l3, mF3.upd bf (frameBytes f4 (fblk.drop 44)), false⟩ := by
              have hm' : mF3 bf = some (frameBytes f3 (fblk.drop 44)) := by simp [mF3]
              have ho : 16 + 8 ≤ (frameBytes f3 (fblk.drop 44)).length := by
                rw [frameBytes_length]; omega
              have he : EvE Gen.prog ⟨l3, mF3, false⟩ eTotalHdr
                  (.int IntTy.u64 (t0 + hdrOf b)) := by
                refine evAddU64 l3 mF3 false t0 (hdrOf b) (.var 10) (.var 7)
                  (EvE.var (by simp [l3, l2, l1, t0])) (EvE.var (by simp [l3, l2, l1, h7])) ?_
                have := limit_lt_two64 b
                have : t0 + hdrOf b < 18446744073709551616 := by omega
                exact this
              have hs := store_field_u64 l3 mF3 bf (frameBytes f3 (fblk.drop 44)) 16
                (t0 + hdrOf b) eTotalHdr (by simp [l3, l2, l1, h3]) hm' ho he
              simpa [b12, f4, frameBytes_binOff] using hs
            -- store bin_len
            have hb13 : ExS Gen.prog ⟨l3, mF3.upd bf (frameBytes f4 (fblk.drop 44)), false⟩ b13 .normal
                ⟨l3, (mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44)), false⟩ := by
              have hm' : (mF3.upd bf (frameBytes f4 (fblk.drop 44))) bf
                  = some (frameBytes f4 (fblk.drop 44)) := by simp
              have ho : 24 + 8 ≤ (frameBytes f4 (fblk.drop 44)).length := by
                rw [frameBytes_length]; omega
              have he : EvE Gen.prog ⟨l3, mF3.upd bf (frameBytes f4 (fblk.drop 44)), false⟩ (.var 9)
                  (.int IntTy.u64 (align4 binLen)) := EvE.var (by simp [l3, l2, l1, t0])
              have hs := store_field_u64 l3 (mF3.upd bf (frameBytes f4 (fblk.drop 44))) bf
                (frameBytes f4 (fblk.drop 44)) 24 (align4 binLen) (.var 9)
                (by simp [l3, l2, l1, h3]) hm' ho he
              simpa [b13, f5, frameBytes_binLen, Mem.upd_upd] using hs
            -- store has_bin
            have hb14 : ExS Gen.prog
                ⟨l3, (mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf (frameBytes f5 (fblk.drop 44)), false⟩
                b14 .normal
                ⟨l3, ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)), false⟩ := by
              have hm' : ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))) bf = some (frameBytes f5 (fblk.drop 44)) := by simp
              have ho : 36 + 4 ≤ (frameBytes f5 (fblk.drop 44)).length := by
                rw [frameBytes_length]; omega
              have he : EvE Gen.prog
                  ⟨l3, (mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf (frameBytes f5 (fblk.drop 44)), false⟩
                  (.const (.int IntTy.i32 1)) (.int IntTy.i32 1) := EvE.const _
              have hs := store_field_i32 l3
                ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf (frameBytes f5 (fblk.drop 44))) bf
                (frameBytes f5 (fblk.drop 44)) 36 1 (.const (.int IntTy.i32 1))
                (by simp [l3, l2, l1, h3]) hm' ho he
              simpa [b14, f6, frameBytes_hasBin_true, Mem.upd_upd] using hs
            -- set the final total
            have hb15 : ExS Gen.prog
                ⟨l3, ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)), false⟩
                b15 .normal
                ⟨l4, ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)), false⟩ := by
              refine ExS.set ?_
              refine evAddU64 l3
                (((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)))
                false (t0 + hdrOf b) (align4 binLen) (.bin .add (some IntTy.u64) (.var 10) (.var 7))
                (.var 9) ?_ (EvE.var (by simp [l3, l2, l1, t0])) ?_
              · refine evAddU64 l3
                  (((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                    (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)))
                  false t0 (hdrOf b) (.var 10) (.var 7)
                  (EvE.var (by simp [l3, l2, l1, t0])) (EvE.var (by simp [l3, l2, l1, h7])) ?_
                have := limit_lt_two64 b
                have : t0 + hdrOf b < 18446744073709551616 := by omega
                exact this
              · have := limit_lt_two64 b
                have : t0 + hdrOf b + align4 binLen < 18446744073709551616 := by omega
                exact this
            -- state after the binary chunk
            have hbinBody : ExS Gen.prog ⟨l2, mF3, false⟩ binBody .normal
                ⟨l4, ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)), false⟩ :=
              ExS.seq hguard1Ite (ExS.seq hcallB (ExS.seq hguard2Ite
                (ExS.seq hb12 (ExS.seq hb13 (ExS.seq hb14 hb15)))))
            have hbinIte : ExS Gen.prog ⟨l2, mF3, false⟩ binCond .normal
                ⟨l4, ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
                  (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44)), false⟩ :=
              ExS.ite (bc := true) hcondBin (by simp [truthy, boolVal, hbin]) hbinBody
            -- write total_out and return 1
            let mFin : Mem := ((mF3.upd bf (frameBytes f4 (fblk.drop 44))).upd bf
              (frameBytes f5 (fblk.drop 44))).upd bf (frameBytes f6 (fblk.drop 44))
            have htot : ExS Gen.prog ⟨l4, mFin, false⟩ s17 .normal
                ⟨l4, mFin.upd bt (splice tblk 0 (encodeLE 8 (t0 + hdrOf b + align4 binLen))), false⟩ := by
              have hm' : mFin bt = some tblk := by
                simp [mFin, mF3, Mem.upd, hne, Ne.symm hne, hmt]
              have he : EvE Gen.prog ⟨l4, mFin, false⟩ (.var 10)
                  (.int IntTy.u64 (t0 + hdrOf b + align4 binLen)) := EvE.var (by simp [l4, l3, l2, l1, t0])
              exact store_ptr_u64 l4 mFin bt tblk (t0 + hdrOf b + align4 binLen) (.var 10)
                (by simp [l4, l3, l2, l1, h4]) hm' hlent he
            have hret : ExS Gen.prog ⟨l4, mFin.upd bt (splice tblk 0 (encodeLE 8 (t0 + hdrOf b + align4 binLen))), false⟩
                ret1 (.ret (some (.int IntTy.i32 1)))
                ⟨l4, mFin.upd bt (splice tblk 0 (encodeLE 8 (t0 + hdrOf b + align4 binLen))), false⟩ :=
              ExS.ret (EvE.const _)
            have hlay := layout_bin_some jsonLen binLen b hbin_ne htotal_le htb_le
            have hcode : (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0) = 1 := by
              simp [hlay]
            refine ⟨⟨l4, mFin.upd bt (splice tblk 0 (encodeLE 8 (t0 + hdrOf b + align4 binLen))), false⟩, ?_, ?_⟩
            · rw [hcode]
              simp only [layoutRest]
              exact ExS.seq hite (ExS.seq hcall (ExS.seq hite2 (ExS.seq hset
                (ExS.seq hstore1 (ExS.seq hstore2 (ExS.seq hstore3
                  (ExS.seq hbinIte (ExS.seq htot hret))))))))
            · intro fr total hfr
              rw [hlay] at hfr
              have hpair := Option.some.inj hfr
              cases hpair
              constructor
              · -- frame fields
                simp [mFin, mF3, Mem.upd, f6, f5, f4, f3, f2, f1, frameBytes, hne, Ne.symm hne, t0]
              · -- total_out
                simp [mFin, Mem.upd, t0]
      · -- binLen = 0
        have hzero : binLen = 0 := by omega
        have hlay := layout_zero_some jsonLen b htotal_le
        have hcode : (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0) = 1 := by
          subst hzero
          simp [hlay]
        have hbinIte : ExS Gen.prog ⟨l2, mF3, false⟩ binCond .normal ⟨l2, mF3, false⟩ :=
          ExS.ite (bc := false) hcondBin (by simp [truthy, boolVal, hbin]) ExS.skip
        have htot : ExS Gen.prog ⟨l2, mF3, false⟩ s17 .normal
            ⟨l2, mF3.upd bt (splice tblk 0 (encodeLE 8 t0)), false⟩ := by
          have hm' : mF3 bt = some tblk := by
            simp [mF3, Mem.upd, hne, Ne.symm hne, hmt]
          have he : EvE Gen.prog ⟨l2, mF3, false⟩ (.var 10) (.int IntTy.u64 t0) :=
            EvE.var (by simp [t0, l2, l1])
          exact store_ptr_u64 l2 mF3 bt tblk t0 (.var 10)
            (by simp [l2, l1, h4]) hm' hlent he
        have hret : ExS Gen.prog ⟨l2, mF3.upd bt (splice tblk 0 (encodeLE 8 t0)), false⟩ ret1
            (.ret (some (.int IntTy.i32 1)))
            ⟨l2, mF3.upd bt (splice tblk 0 (encodeLE 8 t0)), false⟩ :=
          ExS.ret (EvE.const _)
        refine ⟨⟨l2, mF3.upd bt (splice tblk 0 (encodeLE 8 t0)), false⟩, ?_, ?_⟩
        · rw [hcode]
          simp only [layoutRest]
          exact ExS.seq hite (ExS.seq hcall (ExS.seq hite2 (ExS.seq hset
            (ExS.seq hstore1 (ExS.seq hstore2 (ExS.seq hstore3
              (ExS.seq hbinIte (ExS.seq htot hret))))))))
        · intro fr total hfr
          rw [hzero, hlay] at hfr
          have hpair := Option.some.inj hfr
          cases hpair
          constructor
          · simp [mF3, Mem.upd, f3, f2, f1, frameBytes, hne, Ne.symm hne, t0]
          · simp [mF3, Mem.upd, t0]


/-! ## Full function refinement -/

theorem glb_layout_refines (loc : Locals) (m : Mem) (jsonLen binLen : Nat) (b : Bool)
    (bf bt : Nat) (fblk tblk : Block)
    (h0 : loc 0 = some (.int IntTy.u64 jsonLen))
    (h1 : loc 1 = some (.int IntTy.u64 binLen))
    (h2 : loc 2 = some (.int IntTy.i32 (b2n b)))
    (h3 : loc 3 = some (.ptr bf 0))
    (h4 : loc 4 = some (.ptr bt 0))
    (hf : m bf = some fblk) (hmt : m bt = some tblk)
    (hlenf : 44 ≤ fblk.length) (hlent : 8 ≤ tblk.length) (hne : bf ≠ bt)
    (hj : jsonLen < 18446744073709551616) (hbl : binLen < 18446744073709551616) :
    ∃ s', ExS Gen.prog ⟨loc, m, false⟩ Gen.tg3__glb_layout.body
        (.ret (some (.int IntTy.i32
          (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0)))) s' ∧
      (∀ fr total, Glb.layout jsonLen binLen b = some (fr, total) →
        s'.mem bf = some (frameBytes fr (fblk.drop 44)) ∧
        s'.mem bt = some (splice tblk 0 (encodeLE 8 total))) := by
  -- the seven field-clearing stores
  have h0s : ExS Gen.prog ⟨loc, m, false⟩ s0 .normal
      ⟨loc, m.upd bf (splice fblk 0 (encodeLE 8 0)), false⟩ := by
    refine store_field_u64 loc m bf fblk 0 0 eCast0 h3 hf (by omega) ?_
    exact evCastU64I32 loc m false 0 (by decide)
  let b0 := splice fblk 0 (encodeLE 8 0)
  have lb0 : b0.length = fblk.length := by
    change (splice fblk 0 (encodeLE 8 0)).length = fblk.length
    rw [splice_length fblk 0 (encodeLE 8 0) (by rw [encodeLE_length]; omega)]
  have h1s : ExS Gen.prog ⟨loc, m.upd bf b0, false⟩ s1 .normal
      ⟨loc, (m.upd bf b0).upd bf (splice b0 8 (encodeLE 8 0)), false⟩ := by
    have hm : (m.upd bf b0) bf = some b0 := by simp
    have ho : 8 + 8 ≤ b0.length := by rw [lb0]; omega
    refine store_field_u64 loc (m.upd bf b0) bf b0 8 0 eCast0 h3 hm ho ?_
    exact evCastU64I32 loc (m.upd bf b0) false 0 (by decide)
  let b1 := splice b0 8 (encodeLE 8 0)
  have lb1 : b1.length = fblk.length := by
    change (splice b0 8 (encodeLE 8 0)).length = fblk.length
    rw [splice_length b0 8 (encodeLE 8 0) (by rw [lb0]; rw [encodeLE_length]; omega), lb0]
  have h1s' : ExS Gen.prog ⟨loc, m.upd bf b0, false⟩ s1 .normal
      ⟨loc, m.upd bf b1, false⟩ := by
    simpa [b1, Mem.upd_upd] using h1s
  have h2s : ExS Gen.prog ⟨loc, m.upd bf b1, false⟩ s2 .normal
      ⟨loc, m.upd bf (splice b1 16 (encodeLE 8 0)), false⟩ := by
    have hm : (m.upd bf b1) bf = some b1 := by simp
    have ho : 16 + 8 ≤ b1.length := by rw [lb1]; omega
    have hs := store_field_u64 loc (m.upd bf b1) bf b1 16 0 eCast0 h3 hm ho
      (evCastU64I32 loc (m.upd bf b1) false 0 (by decide))
    simpa [s2, Mem.upd_upd] using hs
  let b2 := splice b1 16 (encodeLE 8 0)
  have lb2 : b2.length = fblk.length := by
    change (splice b1 16 (encodeLE 8 0)).length = fblk.length
    rw [splice_length b1 16 (encodeLE 8 0) (by rw [lb1]; rw [encodeLE_length]; omega), lb1]
  have h2s' : ExS Gen.prog ⟨loc, m.upd bf b1, false⟩ s2 .normal
      ⟨loc, m.upd bf b2, false⟩ := by simpa [b2] using h2s
  have h3s : ExS Gen.prog ⟨loc, m.upd bf b2, false⟩ s3 .normal
      ⟨loc, m.upd bf (splice b2 24 (encodeLE 8 0)), false⟩ := by
    have hm : (m.upd bf b2) bf = some b2 := by simp
    have ho : 24 + 8 ≤ b2.length := by rw [lb2]; omega
    have hs := store_field_u64 loc (m.upd bf b2) bf b2 24 0 eCast0 h3 hm ho
      (evCastU64I32 loc (m.upd bf b2) false 0 (by decide))
    simpa [s3, Mem.upd_upd] using hs
  let b3 := splice b2 24 (encodeLE 8 0)
  have lb3 : b3.length = fblk.length := by
    change (splice b2 24 (encodeLE 8 0)).length = fblk.length
    rw [splice_length b2 24 (encodeLE 8 0) (by rw [lb2]; rw [encodeLE_length]; omega), lb2]
  have h3s' : ExS Gen.prog ⟨loc, m.upd bf b2, false⟩ s3 .normal
      ⟨loc, m.upd bf b3, false⟩ := by simpa [b3] using h3s
  have h4s : ExS Gen.prog ⟨loc, m.upd bf b3, false⟩ s4 .normal
      ⟨loc, m.upd bf (splice b3 32 (encodeLE 4 0)), false⟩ := by
    have hm : (m.upd bf b3) bf = some b3 := by simp
    have ho : 32 + 4 ≤ b3.length := by rw [lb3]; omega
    have hs := store_field_i32 loc (m.upd bf b3) bf b3 32 0 (.const (.int IntTy.i32 0))
      h3 hm ho (EvE.const _)
    simpa [s4, Mem.upd_upd] using hs
  let b4 := splice b3 32 (encodeLE 4 0)
  have lb4 : b4.length = fblk.length := by
    change (splice b3 32 (encodeLE 4 0)).length = fblk.length
    rw [splice_length b3 32 (encodeLE 4 0) (by rw [lb3]; rw [encodeLE_length]; omega), lb3]
  have h4s' : ExS Gen.prog ⟨loc, m.upd bf b3, false⟩ s4 .normal
      ⟨loc, m.upd bf b4, false⟩ := by simpa [b4] using h4s
  have h5s : ExS Gen.prog ⟨loc, m.upd bf b4, false⟩ s5 .normal
      ⟨loc, m.upd bf (splice b4 36 (encodeLE 4 0)), false⟩ := by
    have hm : (m.upd bf b4) bf = some b4 := by simp
    have ho : 36 + 4 ≤ b4.length := by rw [lb4]; omega
    have hs := store_field_i32 loc (m.upd bf b4) bf b4 36 0 (.const (.int IntTy.i32 0))
      h3 hm ho (EvE.const _)
    simpa [s5, Mem.upd_upd] using hs
  let b5 := splice b4 36 (encodeLE 4 0)
  have lb5 : b5.length = fblk.length := by
    change (splice b4 36 (encodeLE 4 0)).length = fblk.length
    rw [splice_length b4 36 (encodeLE 4 0) (by rw [lb4]; rw [encodeLE_length]; omega), lb4]
  have h5s' : ExS Gen.prog ⟨loc, m.upd bf b4, false⟩ s5 .normal
      ⟨loc, m.upd bf b5, false⟩ := by simpa [b5] using h5s
  have h6s : ExS Gen.prog ⟨loc, m.upd bf b5, false⟩ s6 .normal
      ⟨loc, m.upd bf (splice b5 40 (encodeLE 4 (b2n b))), false⟩ := by
    have hm : (m.upd bf b5) bf = some b5 := by simp
    have ho : 40 + 4 ≤ b5.length := by rw [lb5]; omega
    have he : EvE Gen.prog ⟨loc, m.upd bf b5, false⟩
        (.cond (.var 2) (.const (.int IntTy.i32 1)) (.const (.int IntTy.i32 0)))
        (.int IntTy.i32 (b2n b)) := evCondIs64 loc (m.upd bf b5) false b (by simp [h2])
    have hs := store_field_i32 loc (m.upd bf b5) bf b5 40 (b2n b)
      (.cond (.var 2) (.const (.int IntTy.i32 1)) (.const (.int IntTy.i32 0))) h3 hm ho he
    simpa [s6, Mem.upd_upd] using hs
  have h6s' : ExS Gen.prog ⟨loc, m.upd bf b5, false⟩ s6 .normal
      ⟨loc, m.upd bf (frameBytes { is64 := b } (fblk.drop 44)), false⟩ := by
    simpa [b0, b1, b2, b3, b4, b5, init_bytes fblk b hlenf] using h6s
  -- store *total_out = 0
  let tblk0 := splice tblk 0 (encodeLE 8 0)
  have ltblk0 : tblk0.length = tblk.length := by
    change (splice tblk 0 (encodeLE 8 0)).length = tblk.length
    rw [splice_length tblk 0 (encodeLE 8 0) (by rw [encodeLE_length]; omega)]
  have h7s : ExS Gen.prog ⟨loc, m.upd bf (frameBytes { is64 := b } (fblk.drop 44)), false⟩
      s7 .normal ⟨loc, (m.upd bf (frameBytes { is64 := b } (fblk.drop 44))).upd bt tblk0, false⟩ := by
    have hm : (m.upd bf (frameBytes { is64 := b } (fblk.drop 44))) bt = some tblk := by
      simp [Mem.upd, hne, Ne.symm hne, hmt]
    have he : EvE Gen.prog ⟨loc, m.upd bf (frameBytes { is64 := b } (fblk.drop 44)), false⟩ eCast0
        (.int IntTy.u64 0) := evCastU64I32 loc _ false 0 (by decide)
    have hs := store_ptr_u64 loc (m.upd bf (frameBytes { is64 := b } (fblk.drop 44))) bt tblk 0
      eCast0 (by simp [h4]) hm hlent he
    simpa [s7, tblk0, Mem.upd_upd] using hs
  -- the is64 branch
  cases b
  · -- 32-bit
    let loc1 : Locals := ((loc.upd 5 (.int IntTy.u64 (limitOf false))).upd 6
      (.int IntTy.u64 (startOf false))).upd 7 (.int IntTy.u64 (hdrOf false))
    have hcond8 : EvE Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩
        (.var 2) (.int IntTy.i32 (b2n false)) := EvE.var (by simp [h2])
    have hs5 : ExS Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩
        (.set 5 eLim32) .normal
        ⟨loc.upd 5 (.int IntTy.u64 (limitOf false)),
          (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.set (evCastU64U64 loc _ false 4294967295 (by decide))
    have hs6 : ExS Gen.prog
        ⟨loc.upd 5 (.int IntTy.u64 (limitOf false)),
          (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩
        (.set 6 eCast12) .normal
        ⟨(loc.upd 5 (.int IntTy.u64 (limitOf false))).upd 6 (.int IntTy.u64 (startOf false)),
          (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.set (evCastU64U32 _ _ false 12 (by decide))
    have hs7 : ExS Gen.prog
        ⟨(loc.upd 5 (.int IntTy.u64 (limitOf false))).upd 6 (.int IntTy.u64 (startOf false)),
          (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩
        (.set 7 eCast8) .normal
        ⟨loc1, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.set (evCastU64U32 _ _ false 8 (by decide))
    have hchain8 : ExS Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩
        (.seq (.set 5 eLim32) (.seq (.set 6 eCast12) (.set 7 eCast8))) .normal
        ⟨loc1, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.seq hs5 (ExS.seq hs6 hs7)
    have h8s : ExS Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩
        s8 .normal
        ⟨loc1, (m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.ite (bc := false) hcond8 (by simp [truthy, b2n]) hchain8
    obtain ⟨s', hrest, hprop⟩ := layout_rest loc1
      ((m.upd bf (frameBytes { is64 := false } (fblk.drop 44))).upd bt tblk0)
      jsonLen binLen false bf bt fblk tblk0
      (by simp [loc1, h0]) (by simp [loc1, h1]) (by simp [loc1, h3]) (by simp [loc1, h4])
      (by simp [loc1, limitOf, startOf, hdrOf]) (by simp [loc1, limitOf, startOf, hdrOf])
      (by simp [loc1, limitOf, startOf, hdrOf])
      (by simp [Mem.upd, hne, Ne.symm hne])
      (by simp)
      hlenf (by rw [ltblk0]; exact hlent) hne hj hbl
    refine ⟨s', ?_, ?_⟩
    · rw [body_structure]
      exact ExS.seq h0s (ExS.seq h1s' (ExS.seq h2s' (ExS.seq h3s' (ExS.seq h4s'
        (ExS.seq h5s' (ExS.seq h6s' (ExS.seq h7s (ExS.seq h8s hrest))))))))
    · intro fr total hlay
      obtain ⟨hb, ht⟩ := hprop fr total hlay
      rw [splice_splice_zero tblk (encodeLE 8 0) (encodeLE 8 total) (by simp)] at ht
      exact ⟨hb, ht⟩
  · -- 64-bit
    let loc1 : Locals := ((loc.upd 5 (.int IntTy.u64 (limitOf true))).upd 6
      (.int IntTy.u64 (startOf true))).upd 7 (.int IntTy.u64 (hdrOf true))
    have hcond8 : EvE Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩
        (.var 2) (.int IntTy.i32 (b2n true)) := EvE.var (by simp [h2])
    have hs5 : ExS Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩
        (.set 5 eLim64) .normal
        ⟨loc.upd 5 (.int IntTy.u64 (limitOf true)),
          (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.set (evCastU64U64 loc _ false 18446744073709551615 (by decide))
    have hs6 : ExS Gen.prog
        ⟨loc.upd 5 (.int IntTy.u64 (limitOf true)),
          (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩
        (.set 6 eCast16) .normal
        ⟨(loc.upd 5 (.int IntTy.u64 (limitOf true))).upd 6 (.int IntTy.u64 (startOf true)),
          (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.set (evCastU64U32 _ _ false 16 (by decide))
    have hs7 : ExS Gen.prog
        ⟨(loc.upd 5 (.int IntTy.u64 (limitOf true))).upd 6 (.int IntTy.u64 (startOf true)),
          (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩
        (.set 7 eCast12) .normal
        ⟨loc1, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.set (evCastU64U32 _ _ false 12 (by decide))
    have hchain8 : ExS Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩
        (.seq (.set 5 eLim64) (.seq (.set 6 eCast16) (.set 7 eCast12))) .normal
        ⟨loc1, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.seq hs5 (ExS.seq hs6 hs7)
    have h8s : ExS Gen.prog
        ⟨loc, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩
        s8 .normal
        ⟨loc1, (m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0, false⟩ :=
      ExS.ite (bc := true) hcond8 (by simp [truthy, b2n]) hchain8
    obtain ⟨s', hrest, hprop⟩ := layout_rest loc1
      ((m.upd bf (frameBytes { is64 := true } (fblk.drop 44))).upd bt tblk0)
      jsonLen binLen true bf bt fblk tblk0
      (by simp [loc1, h0]) (by simp [loc1, h1]) (by simp [loc1, h3]) (by simp [loc1, h4])
      (by simp [loc1, limitOf, startOf, hdrOf]) (by simp [loc1, limitOf, startOf, hdrOf])
      (by simp [loc1, limitOf, startOf, hdrOf])
      (by simp [Mem.upd, hne, Ne.symm hne])
      (by simp)
      hlenf (by rw [ltblk0]; exact hlent) hne hj hbl
    refine ⟨s', ?_, ?_⟩
    · rw [body_structure]
      exact ExS.seq h0s (ExS.seq h1s' (ExS.seq h2s' (ExS.seq h3s' (ExS.seq h4s'
        (ExS.seq h5s' (ExS.seq h6s' (ExS.seq h7s (ExS.seq h8s hrest))))))))
    · intro fr total hlay
      obtain ⟨hb, ht⟩ := hprop fr total hlay
      rw [splice_splice_zero tblk (encodeLE 8 0) (encodeLE 8 total) (by simp)] at ht
      exact ⟨hb, ht⟩

/-- **T1 (layout), end-to-end.** `tg3__glb_layout` returns 1 exactly when
    `Glb.layout` succeeds (0 otherwise), and on success leaves exactly the
    specified frame in `f` and the total size in `*total_out`. -/
theorem glb_layout_correct (m : Mem) (jsonLen binLen : Nat) (b : Bool)
    (bf bt : Nat) (fblk tblk : Block)
    (hf : m bf = some fblk) (hmt : m bt = some tblk)
    (hlenf : 44 ≤ fblk.length) (hlent : 8 ≤ tblk.length) (hne : bf ≠ bt)
    (hj : jsonLen < 18446744073709551616) (hbl : binLen < 18446744073709551616) :
    ∃ m' K, (∀ n, K ≤ n → callFunc Gen.prog n Gen.fid_tg3__glb_layout
        [.int IntTy.u64 jsonLen, .int IntTy.u64 binLen, .int IntTy.i32 (b2n b),
         .ptr bf 0, .ptr bt 0] m =
        .ok (some (.int IntTy.i32
          (match Glb.layout jsonLen binLen b with | some _ => 1 | none => 0)), m')) ∧
      (∀ fr total, Glb.layout jsonLen binLen b = some (fr, total) →
        m' bf = some (frameBytes fr (fblk.drop 44)) ∧
        m' bt = some (splice tblk 0 (encodeLE 8 total))) := by
  let loc : Locals := frameLocals [0, 1, 2, 3, 4]
    [.int IntTy.u64 jsonLen, .int IntTy.u64 binLen, .int IntTy.i32 (b2n b), .ptr bf 0, .ptr bt 0]
  have h0 : loc 0 = some (.int IntTy.u64 jsonLen) := by
    simp [loc, frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  have h1 : loc 1 = some (.int IntTy.u64 binLen) := by
    simp [loc, frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  have h2 : loc 2 = some (.int IntTy.i32 (b2n b)) := by
    simp [loc, frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  have h3 : loc 3 = some (.ptr bf 0) := by
    simp [loc, frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  have h4 : loc 4 = some (.ptr bt 0) := by
    simp [loc, frameLocals, List.idxOf?, List.findIdx?, List.findIdx?.go]
  obtain ⟨s', hex, hprop⟩ := glb_layout_refines loc m jsonLen binLen b bf bt fblk tblk
    h0 h1 h2 h3 h4 hf hmt hlenf hlent hne hj hbl
  obtain ⟨K, hK⟩ := callFunc_of_ExS (p := Gen.prog) (f := Gen.fid_tg3__glb_layout)
    (fn := Gen.tg3__glb_layout) rfl rfl hex
  exact ⟨s'.mem, K, hK, hprop⟩

end GlTF.C.Proofs
