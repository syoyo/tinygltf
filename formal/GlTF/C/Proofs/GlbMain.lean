/-
  T1: `tg3__glb_frame_parse` refines `Glb.parseFrame` (main theorem).
-/
import GlTF.C.Proofs.Loop
open GlTF GlTF.C GlTF.C.Proofs
set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false
set_option linter.deprecated false
set_option maxRecDepth 4000

namespace GlTF.C.Proofs

/-- Drop the first `k` statements of a right-nested sequence. -/
def dropSeq : Nat → Stmt → Stmt
  | 0, s => s
  | k + 1, .seq _ b => dropSeq k b
  | _, s => s

/-- The function body from the `total != size` check onwards. -/
def tailStmt : Stmt := dropSeq 11 Gen.tg3__glb_frame_parse.body

theorem tail_eq : tailStmt =
    .seq (.ite (.bin .ne (some ⟨64, false⟩) (.var 5) (.var 1))
            (.ret (some (.cast ⟨32, false⟩ (.const (.int ⟨32, true⟩ 24))))) .skip)
      (.seq Gen.tg3__glb_frame_parse_loop0
        (.seq (.ite (.un .lnot ⟨32, true⟩ (.load (.int ⟨32, true⟩) (.field (.var 2) 32)))
                (.ret (some (.cast ⟨32, false⟩ (.const (.int ⟨32, true⟩ 23))))) .skip)
          (.ret (some (.cast ⟨32, false⟩ (.const (.int ⟨32, true⟩ 0))))))) := rfl

theorem tail_exs (m : Mem) (bd bf : Nat) (d t : List UInt8) (hne : bd ≠ bf)
    (hl : d.length < 18446744073709551616) (ht : t.length = 4)
    (hdr off total : Nat) (fr : Glb.Frame) (s : State) (hh : hdr = 8 ∨ hdr = 12)
    (hs : LoopFacts m bd bf d t hdr off fr s) (h5 : s.locals 5 = some (.int u64 total))
    (hoff : off ≤ d.length) :
    ∃ c s', ExS Gen.prog s tailStmt (.ret (some (.int u32 c))) s' ∧ ExitFacts m bd bf d s' ∧
      c = (if total = d.length then code (Glb.loop d hdr (d.length + 1) off fr) else 24) ∧
      (total = d.length → ∀ f, Glb.loop d hdr (d.length + 1) off fr = .ok f →
        s'.mem bf = some (frameBytes f t)) := by
  have l1 := hs.l1
  rw [tail_eq]
  by_cases htot : total = d.length
  · have hc : EvE Gen.prog s (.bin .ne (some ⟨64, false⟩) (.var 5) (.var 1)) (boolVal false) :=
      EvE.of_eq 5 (fun n => by msimp)
    obtain ⟨o, s', hL, hE, hres⟩ := loop_exs m bd bf d t hne hl ht (d.length + 1) off fr s hdr hh hs hoff
      (by omega)
    rcases hres with ⟨rfl, hspec⟩ | ⟨rfl, fr', hmbf, hspec⟩
    · refine ⟨23, s', ExS.seq (ExS.ite hc rfl ExS.skip) (ExS.seq_ret hL), hE, ?_, ?_⟩
      · simp [htot, hspec, code]
      · intro _ f hf; rw [hspec] at hf; cases hf
    · have hE2 := hE
      have l2 := hE.l2
      by_cases hj : fr'.hasJson
      · have hc2 : EvE Gen.prog s' (.un .lnot ⟨32, true⟩ (.load (.int ⟨32, true⟩) (.field (.var 2) 32)))
            (boolVal false) := EvE.of_eq 5 (fun n => by msimp [ht])
        refine ⟨0, s', ExS.seq (ExS.ite hc rfl ExS.skip) (ExS.seq hL
          (ExS.seq (ExS.ite hc2 rfl ExS.skip) (ExS.ret (EvE.of_eq 5 (fun n => by msimp))))), hE, ?_, ?_⟩
        · simp [htot, hspec, hj, code]
        · intro _ f hf; simp [hspec, hj] at hf; subst hf; exact hmbf
      · have hc2 : EvE Gen.prog s' (.un .lnot ⟨32, true⟩ (.load (.int ⟨32, true⟩) (.field (.var 2) 32)))
            (boolVal true) := EvE.of_eq 5 (fun n => by msimp [ht])
        refine ⟨23, s', ExS.seq (ExS.ite hc rfl ExS.skip) (ExS.seq hL
          (ExS.seq_ret (ExS.ite hc2 rfl (ExS.ret (EvE.of_eq 5 (fun n => by msimp)))))), hE, ?_, ?_⟩
        · simp [htot, hspec, hj, code]
        · intro _ f hf; simp [hspec, hj] at hf
  · have hc : EvE Gen.prog s (.bin .ne (some ⟨64, false⟩) (.var 5) (.var 1)) (boolVal true) :=
      EvE.of_eq 5 (fun n => by msimp)
    refine ⟨24, s, ExS.seq_ret (ExS.ite hc rfl (ExS.ret (EvE.of_eq 5 (fun n => by msimp)))),
      ⟨hs.l2, hs.mbd, hs.ro, hs.other⟩, by simp [htot], by intro h; exact absurd h htot⟩

theorem finish_v (m : Mem) (bd bf : Nat) (d t : List UInt8) (S : State) (hdr off total : Nat)
    (fr : Glb.Frame) (X : Except Glb.Err Glb.Frame)
    (hX : X = if total ≠ d.length then .error .sizeMismatch else Glb.loop d hdr (d.length + 1) off fr)
    (h : ∃ c s', ExS Gen.prog S tailStmt (.ret (some (.int u32 c))) s' ∧ ExitFacts m bd bf d s' ∧
      c = (if total = d.length then code (Glb.loop d hdr (d.length + 1) off fr) else 24) ∧
      (total = d.length → ∀ f, Glb.loop d hdr (d.length + 1) off fr = .ok f →
        s'.mem bf = some (frameBytes f t))) :
    ∃ s', ExS Gen.prog S tailStmt (.ret (some (.int u32 (code X)))) s' ∧ ExitFacts m bd bf d s' ∧
      ∀ f, X = .ok f → s'.mem bf = some (frameBytes f t) := by
  obtain ⟨c, s', hT, hE, hc, hok⟩ := h
  subst hX
  by_cases htot : total = d.length
  · simp only [htot, ne_eq, not_true_eq_false, if_false, if_true] at hc hok ⊢
    subst hc
    exact ⟨s', hT, hE, hok trivial⟩
  · simp only [htot, ne_eq, not_false_eq_true, if_true, if_false] at hc ⊢
    subst hc
    exact ⟨s', hT, hE, fun f hf => by cases hf⟩

macro "exit_facts" : tactic => `(tactic| first
  | (intro f hf; cases hf)
  | (intro b hb; simp [Mem.upd_ne _ _ hb, Locals.upd])
  | (simp [List.idxOf?, List.findIdx?, List.findIdx?.go, Locals.upd, *]))

/-- Locals of the call `tg3__glb_frame_parse(data, size, f)`. -/
def entryLocals (bd bf N : Nat) : Locals := frameLocals [0, 1, 2] [.ptr bd 0, .int u64 N, .ptr bf 0]

set_option maxHeartbeats 10000000 in
/-- **T1 (parse).** Running the translated C of `tg3__glb_frame_parse` on any
    byte string `d` returns the error code of `Glb.parseFrame d` without
    undefined behaviour, only writes the frame object, and on success leaves
    exactly the specified frame in it. -/
theorem frame_parse_correct (m : Mem) (bd bf : Nat) (d blk : List UInt8)
    (hm : m bd = some d) (hf : m bf = some blk) (hb : blk.length = 48) (hne : bd ≠ bf)
    (hl : d.length < 18446744073709551616) :
    ∃ s', ExS Gen.prog ⟨entryLocals bd bf d.length, m, false⟩ Gen.tg3__glb_frame_parse.body
        (.ret (some (.int u32 (code (Glb.parseFrame d))))) s' ∧
      ExitFacts m bd bf d s' ∧
      ∀ f, Glb.parseFrame d = .ok f → s'.mem bf = some (frameBytes f (blk.drop 44)) := by
  have hne' : bf ≠ bd := Ne.symm hne
  have ht : (blk.drop 44).length = 4 := by simp [hb]
  simp only [Gen.tg3__glb_frame_parse, entryLocals]
  refine ExS.seq_ex (s1 := ?u0) (ExS.of_eq 10 (fun n => ?a0)) ?b0
  case a0 => msimp; rfl
  refine ExS.seq_ex (s1 := ?u1) (ExS.of_eq 10 (fun n => ?a1)) ?b1
  case a1 => msimp; rfl
  refine ExS.seq_ex (s1 := ?u2) (ExS.of_eq 10 (fun n => ?a2)) ?b2
  case a2 => msimp; rfl
  refine ExS.seq_ex (s1 := ?u3) (ExS.of_eq 10 (fun n => ?a3)) ?b3
  case a3 => msimp; rfl
  refine ExS.seq_ex (s1 := ?u4) (ExS.of_eq 10 (fun n => ?a4)) ?b4
  case a4 => msimp; rfl
  refine ExS.seq_ex (s1 := ?u5) (ExS.of_eq 10 (fun n => ?a5)) ?b5
  case a5 => msimp; rfl
  refine ExS.seq_ex (s1 := ?u6) (ExS.of_eq 10 (fun n => ?a6)) ?b6
  case a6 => msimp; rfl
  -- size < 12
  by_cases h12 : d.length < 12
  · have hp : Glb.parseFrame d = .error .header := by simp [Glb.parseFrame, h12]
    rw [hp]
    refine ⟨?sA, ExS.seq_ret (ExS.of_eq 10 (fun n => ?rA)), ⟨?_, ?_, ?_, ?_⟩, ?_⟩
    case rA => msimp [code]; rfl
    all_goals exit_facts
  refine ExS.seq_ex (s1 := ?v0) (ExS.of_eq 10 (fun n => ?c0)) ?d0
  case c0 => msimp; rfl
  -- magic
  have hb0 := decodeLE_take_one_drop d 0 (by omega)
  have hb1 := decodeLE_take_one_drop d 1 (by omega)
  have hb2 := decodeLE_take_one_drop d 2 (by omega)
  have hb3 := decodeLE_take_one_drop d 3 (by omega)
  by_cases hmag : Glb.byteAt d 0 = 0x67 ∧ Glb.byteAt d 1 = 0x6C ∧ Glb.byteAt d 2 = 0x54 ∧
      Glb.byteAt d 3 = 0x46
  case neg =>
    have hp : Glb.parseFrame d = .error .magic := by simp [Glb.parseFrame, h12, hmag]
    rw [hp]
    refine ⟨?sB, ExS.seq_ret (ExS.of_eq 10 (fun n => ?rB)), ⟨?_, ?_, ?_, ?_⟩, ?_⟩
    case rB =>
      have : ¬ (Glb.byteAt d 0 = 103 ∧ Glb.byteAt d 1 = 108 ∧ Glb.byteAt d 2 = 84 ∧
        Glb.byteAt d 3 = 70) := hmag
      have i1 : (1 : Int) ≤ d.length := by omega
      have i2 : (2 : Int) ≤ d.length := by omega
      have i3 : (3 : Int) ≤ d.length := by omega
      have i4 : (4 : Int) ≤ d.length := by omega
      by_cases b0 : Glb.byteAt d 0 = 103 <;> by_cases b1 : Glb.byteAt d 1 = 108 <;>
      by_cases b2 : Glb.byteAt d 2 = 84 <;> by_cases b3 : Glb.byteAt d 3 = 70 <;>
      msimp [code, decodeLE_take_one, decodeLE_take_one_tail, (by omega : 0 < d.length), (by omega : 1 < d.length),
        (by omega : 2 < d.length), (by omega : 3 < d.length), (by omega : 1 ≤ d.length),
        (by omega : 2 ≤ d.length), (by omega : 3 ≤ d.length), (by omega : 4 ≤ d.length),
        (by omega : 1 + 1 ≤ d.length), (by omega : 2 + 1 ≤ d.length), (by omega : 3 + 1 ≤ d.length)]
        <;> first | rfl | (exfalso; exact this ⟨b0, b1, b2, b3⟩)
    all_goals exit_facts
  obtain ⟨b0, b1, b2, b3⟩ := hmag
  refine ExS.seq_ex (s1 := ?v1) (ExS.of_eq 10 (fun n => ?c1)) ?d1
  case c1 =>
    have i1 : (1 : Int) ≤ d.length := by omega
    have i2 : (2 : Int) ≤ d.length := by omega
    have i3 : (3 : Int) ≤ d.length := by omega
    have i4 : (4 : Int) ≤ d.length := by omega
    msimp [decodeLE_take_one, decodeLE_take_one_tail, (by omega : 0 < d.length), (by omega : 1 < d.length),
      (by omega : 2 < d.length), (by omega : 3 < d.length), (by omega : 1 ≤ d.length),
      (by omega : 2 ≤ d.length), (by omega : 3 ≤ d.length), (by omega : 4 ≤ d.length),
      (by omega : 1 + 1 ≤ d.length), (by omega : 2 + 1 ≤ d.length), (by omega : 3 + 1 ≤ d.length)]
    rfl
  -- version = rd32(data, 4)
  have hfm : ∀ x : Block, (m.upd bf x) bd = some d := fun x => by simp [hm, hne]
  refine ExS.seq_ex (ExS.call_dst (EvArgs.cons (EvE.var (by simp [List.idxOf?, List.findIdx?,
      List.findIdx?.go])) (EvArgs.cons (EvE.of_eq 5 (fun n => by msimp)) EvArgs.nil)) rfl rfl
    (rd32_exs _ bd d (hfm _) 4 (by omega) hl _)) ?d2
  have hmagic : Glb.byteAt d 0 = 0x67 ∧ Glb.byteAt d 1 = 0x6C ∧ Glb.byteAt d 2 = 0x54 ∧
      Glb.byteAt d 3 = 0x46 := ⟨b0, b1, b2, b3⟩
  have hlt := rd32_lt d 4
  by_cases hv2 : Glb.rd32 d 4 = 2
  · -- GLB v2
    have hp : Glb.parseFrame d =
        if Glb.rd32 d 8 ≠ d.length then .error .sizeMismatch else Glb.loop d 8 (d.length + 1) 12 {} := by
      simp only [Glb.parseFrame, h12, hmagic, hv2, if_false, if_true, and_self]
    rw [hp]
    refine ExS.seq_ex (ExS.ite (vc := boolVal true) (bc := true) (EvE.of_eq 5 (fun n => by msimp)) rfl
      (ExS.seq (ExS.set (EvE.cast (v := .int u64 (Glb.rd32 d 8)) (rd32_call (bd := bd) (d := d) (by simp [hm, hne]) (o := 8) (by omega) hl
          (EvE.var (by simp [List.idxOf?, List.findIdx?, List.findIdx?.go, Locals.upd]))
          (EvE.of_eq 5 (fun n => by msimp))) (by simp [evalCast, IntTy.modulus, IntTy.u32, IntTy.u64])))
        (ExS.seq (ExS.set (EvE.of_eq (v := .int u64 12) 5 (fun n => by msimp)))
          (ExS.set (EvE.of_eq (v := .int u64 8) 5 (fun n => by msimp)))))) ?t2
    case t2 =>
      refine finish_v m bd bf d _ _ 8 12 (Glb.rd32 d 8) {} _ rfl
        (tail_exs m bd bf d _ hne hl ht 8 12 _ {} _ (Or.inl rfl) ?facts (by simp [Locals.upd]) (by omega))
      constructor <;> try simp [List.idxOf?, List.findIdx?, List.findIdx?.go, Locals.upd, hm, hne, hne',
        frameBytes, b2n, u64, IntTy.u64]
      intro b hb; simp [Mem.upd_ne _ _ hb]
  by_cases hv3 : Glb.rd32 d 4 = 3
  · -- GLB v3 (glTF 2.1, spec-unstable)
    by_cases h16 : d.length < 16
    · have hp : Glb.parseFrame d = .error .header := by
        simp [Glb.parseFrame, h12, hmagic, hv2, hv3, h16]
      rw [hp]
      refine ⟨?sC, ExS.seq_ret (ExS.ite (EvE.of_eq (v := boolVal false) 5 (fun n => by msimp [hv2])) rfl (ExS.ite (EvE.of_eq (v := boolVal true) 5 (fun n => by msimp [hv3])) rfl
        (ExS.seq_ret (ExS.of_eq 10 (fun n => ?rC))))), ⟨?_, ?_, ?_, ?_⟩, ?_⟩
      case rC => msimp [code]; rfl
      all_goals exit_facts
    · have hp : Glb.parseFrame d =
          if Glb.rd64 d 8 ≠ d.length then .error .sizeMismatch
          else Glb.loop d 12 (d.length + 1) 16 { is64 := true } := by
        simp [Glb.parseFrame, h12, hmagic, hv2, hv3, h16]
      rw [hp]
      refine ExS.seq_ex (s1 := ?w3) (ExS.ite (EvE.of_eq (v := boolVal false) 5 (fun n => by msimp [hv2])) rfl (ExS.ite (EvE.of_eq (v := boolVal true) 5 (fun n => by msimp [hv3])) rfl
        (ExS.seq (ExS.ite (EvE.of_eq (v := boolVal false) 5 (fun n => by msimp)) rfl ExS.skip)
          (ExS.seq (ExS.call_dst (EvArgs.cons (EvE.var (by simp [List.idxOf?, List.findIdx?,
              List.findIdx?.go, Locals.upd])) (EvArgs.cons (EvE.of_eq (v := .int u64 8) 5
              (fun n => by msimp)) EvArgs.nil)) rfl rfl
              (rd64_exs _ bd d (by simp [hm, hne]) 8 (by omega) hl _))
            (ExS.seq (ExS.set (EvE.of_eq (v := .int u64 16) 5 (fun n => by msimp)))
              (ExS.seq (ExS.set (EvE.of_eq (v := .int u64 12) 5 (fun n => by msimp)))
                (ExS.of_eq 10 (fun n => ?st40)))))))) ?t3
      case st40 => msimp; rfl
      case t3 =>
        refine finish_v m bd bf d _ _ 12 16 (Glb.rd64 d 8) { is64 := true } _ rfl
          (tail_exs m bd bf d _ hne hl ht 12 16 _ _ _ (Or.inr rfl) ?facts3 (by simp [Locals.upd] <;> rfl) (by omega))
        constructor <;> (try simp [List.idxOf?, List.findIdx?, List.findIdx?.go, Locals.upd, hm, hne, hne',
          frameBytes, b2n, u64, IntTy.u64]) <;> (try rfl)
        intro b hb; simp [Mem.upd_ne _ _ hb]
  · -- unsupported GLB version
    have hp : Glb.parseFrame d = .error .version := by
      simp only [Glb.parseFrame, h12, hmagic, hv2, hv3, if_false, if_true, and_self]
    rw [hp]
    refine ⟨?sD, ExS.seq_ret (ExS.ite (EvE.of_eq (v := boolVal false) 5 (fun n => by msimp [hv2])) rfl (ExS.ite (EvE.of_eq (v := boolVal false) 5 (fun n => by msimp [hv3])) rfl
      (ExS.of_eq 10 (fun n => ?rD)))), ⟨?_, ?_, ?_, ?_⟩, ?_⟩
    case rD => msimp [code]; rfl
    all_goals exit_facts

end GlTF.C.Proofs

namespace GlTF.C.Proofs

/-- **T1, end-to-end.** For every input `d` and every caller memory in which
    `data` points to `d` and `f` points to a 48-byte `tg3__glb_frame` object
    (distinct from the input), calling the translated C function
    `tg3__glb_frame_parse(data, size, f)` terminates without undefined
    behaviour and returns the `tg3_error_code` of `Glb.parseFrame d`; it
    writes nothing outside `*f`, leaves the input intact, and on success `*f`
    holds exactly the frame computed by the specification (so, by
    `Glb.parseFrame_ok_in`, every chunk it reports lies inside the input). -/
theorem glb_frame_parse_refines (m : Mem) (bd bf : Nat) (d blk : List UInt8)
    (hm : m bd = some d) (hf : m bf = some blk) (hb : blk.length = 48) (hne : bd ≠ bf)
    (hl : d.length < 2 ^ 64) :
    ∃ m' K, (∀ n, K ≤ n → callFunc Gen.prog n Gen.fid_tg3__glb_frame_parse
        [.ptr bd 0, .int u64 d.length, .ptr bf 0] m =
        .ok (some (.int u32 (code (Glb.parseFrame d))), m')) ∧
      m' bd = some d ∧ (∀ b, b ≠ bf → m' b = m b) ∧
      (∀ fr, Glb.parseFrame d = .ok fr → m' bf = some (frameBytes fr (blk.drop 44))) := by
  obtain ⟨s', hex, hE, hok⟩ := frame_parse_correct m bd bf d blk hm hf hb hne (by simpa using hl)
  obtain ⟨K, hK⟩ := callFunc_of_ExS (p := Gen.prog) (f := Gen.fid_tg3__glb_frame_parse)
    (fn := Gen.tg3__glb_frame_parse) rfl rfl hex
  exact ⟨s'.mem, K, hK, hE.mbd, hE.other, hok⟩

end GlTF.C.Proofs
