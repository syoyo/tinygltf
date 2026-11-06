/-
  T1 (write): the byte writers `tg3__wr_u32le` / `tg3__wr_u64le` write exactly
  the little-endian encoding of their argument.
-/
import GlTF.C.Splice
import GlTF.C.Proofs.GlbParse

namespace GlTF.C.Proofs
open GlTF GlTF.C

/-- Storing one byte overwrites exactly that byte. -/
theorem store_byte (m : Mem) (b o : Nat) (blk : Block) (hm : m b = some blk) (v : Nat)
    (ho : o < blk.length) (hv : v < 256) :
    C.store m (.int IntTy.u8) (.ptr b o) (.int IntTy.u8 v)
      = .ok (m.upd b (splice blk o [v.toUInt8])) := by
  have hb : IntTy.u8.bytes = 1 := rfl
  rw [store_int m b o blk hm .u8 v (by rw [hb]; omega), hb]
  simp only [encodeLE, Nat.mod_eq_of_lt hv]

theorem div256 (v : Nat) : v / 256 / 256 = v / 65536 := by
  rw [Nat.div_div_eq_div_mul, show (256 * 256 : Nat) = 65536 from rfl]

theorem div256_3 (v : Nat) : v / 65536 / 256 = v / 16777216 := by
  rw [Nat.div_div_eq_div_mul, show (65536 * 256 : Nat) = 16777216 from rfl]

/-- The four bytes of `encodeLE 4 v`. -/
theorem encodeLE4_eq (v : Nat) :
    encodeLE 4 v = [((v / 1) % 256).toUInt8, ((v / 256) % 256).toUInt8,
                    ((v / 65536) % 256).toUInt8, ((v / 16777216) % 256).toUInt8] := by
  simp only [encodeLE, Nat.div_one, div256, div256_3]

/-- Four consecutive byte stores equal one 4-byte `splice`. -/
theorem stores4 (m : Mem) (b : Nat) (blk : Block) (hm : m b = some blk)
    (o b0 b1 b2 b3 : Nat) (h0 : b0 < 256) (h1 : b1 < 256) (h2 : b2 < 256) (h3 : b3 < 256)
    (hlt : o + 4 ≤ blk.length) :
    (C.store m (.int IntTy.u8) (.ptr b o) (.int IntTy.u8 b0) >>= fun m1 =>
      C.store m1 (.int IntTy.u8) (.ptr b (o + 1)) (.int IntTy.u8 b1) >>= fun m2 =>
      C.store m2 (.int IntTy.u8) (.ptr b (o + 2)) (.int IntTy.u8 b2) >>= fun m3 =>
      C.store m3 (.int IntTy.u8) (.ptr b (o + 3)) (.int IntTy.u8 b3))
      = .ok (m.upd b (splice blk o [b0.toUInt8, b1.toUInt8, b2.toUInt8, b3.toUInt8])) := by
  have hL : ∀ k : Nat, k ≤ 3 → o + k < blk.length := by intro k _; omega
  have hs1 : splice (splice blk o [b0.toUInt8]) (o + 1) [b1.toUInt8]
      = splice blk o ([b0.toUInt8] ++ [b1.toUInt8]) :=
    splice_append blk o [b0.toUInt8] [b1.toUInt8] (by show o + 1 + 1 ≤ blk.length; omega)
  have hs2 : splice (splice blk o ([b0.toUInt8] ++ [b1.toUInt8])) (o + 2) [b2.toUInt8]
      = splice blk o (([b0.toUInt8] ++ [b1.toUInt8]) ++ [b2.toUInt8]) :=
    splice_append blk o _ [b2.toUInt8] (by show o + 2 + 1 ≤ blk.length; omega)
  have hs3 : splice (splice blk o (([b0.toUInt8] ++ [b1.toUInt8]) ++ [b2.toUInt8])) (o + 3) [b3.toUInt8]
      = splice blk o ((([b0.toUInt8] ++ [b1.toUInt8]) ++ [b2.toUInt8]) ++ [b3.toUInt8]) :=
    splice_append blk o _ [b3.toUInt8] (by show o + 3 + 1 ≤ blk.length; omega)
  rw [store_byte m b o blk hm b0 (by show o < blk.length; omega) h0]
  simp only [Res.bind_eq, Res.bind_ok, Mem.upd_same, Mem.upd_upd]
  rw [store_byte (m.upd b (splice blk o [b0.toUInt8])) b (o + 1) (splice blk o [b0.toUInt8])
    (Mem.upd_same _ _ _) b1 (by have := splice_length blk o [b0.toUInt8] (by simp; omega); omega) h1, hs1]
  simp only [Res.bind_eq, Res.bind_ok, Mem.upd_same, Mem.upd_upd]
  rw [store_byte (m.upd b (splice blk o ([b0.toUInt8] ++ [b1.toUInt8]))) b (o + 2)
    (splice blk o ([b0.toUInt8] ++ [b1.toUInt8])) (Mem.upd_same _ _ _) b2
    (by have := splice_length blk o ([b0.toUInt8] ++ [b1.toUInt8]) (by simp; omega); omega) h2, hs2]
  simp only [Res.bind_eq, Res.bind_ok, Mem.upd_same, Mem.upd_upd]
  rw [store_byte (m.upd b (splice blk o (([b0.toUInt8] ++ [b1.toUInt8]) ++ [b2.toUInt8]))) b (o + 3)
    (splice blk o (([b0.toUInt8] ++ [b1.toUInt8]) ++ [b2.toUInt8])) (Mem.upd_same _ _ _) b3
    (by have := splice_length blk o (([b0.toUInt8] ++ [b1.toUInt8]) ++ [b2.toUInt8]) (by simp; omega); omega) h3, hs3]
  simp [Mem.upd_upd, Mem.upd_same, Res.bind_ok]

end GlTF.C.Proofs
