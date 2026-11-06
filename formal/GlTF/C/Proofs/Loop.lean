/-
  T1 (part): the chunk loop of `tg3__glb_frame_parse` refines `Glb.loop`.
-/
import GlTF.C.Proofs.GlbParse
import GlTF.Glb
open GlTF GlTF.C GlTF.C.Proofs
set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false
set_option maxRecDepth 4000
set_option linter.deprecated false

namespace GlTF.C.Proofs

def M64 : Nat := 18446744073709551616

/-- What the loop knows about the C state. -/
structure LoopFacts (m : Mem) (bd bf : Nat) (d t : List UInt8) (hdr off : Nat) (fr : Glb.Frame)
    (s : State) : Prop where
  l0 : s.locals 0 = some (.ptr bd 0)
  l1 : s.locals 1 = some (.int u64 d.length)
  l2 : s.locals 2 = some (.ptr bf 0)
  l6 : s.locals 6 = some (.int u64 off)
  l7 : s.locals 7 = some (.int u64 hdr)
  mbd : s.mem bd = some d
  mbf : s.mem bf = some (frameBytes fr t)
  ro : s.ro = false
  other : ∀ b, b ≠ bf → s.mem b = m b

/-- What remains true when the function returns. -/
structure ExitFacts (m : Mem) (bd bf : Nat) (d : List UInt8) (s : State) : Prop where
  l2 : s.locals 2 = some (.ptr bf 0)
  mbd : s.mem bd = some d
  ro : s.ro = false
  other : ∀ b, b ≠ bf → s.mem b = m b

section
variable (m : Mem) (bd bf : Nat) (d t : List UInt8) (hne : bd ≠ bf)
  (hl : d.length < 18446744073709551616) (ht : t.length = 4)

/-- The loop body after the condition (`B` in `while (c) B`). -/
def loopBody : Stmt :=
  match Gen.tg3__glb_frame_parse_loop0 with
  | .loop (.ite _ b _) _ => b
  | _ => .skip

def loopCond : Expr :=
  match Gen.tg3__glb_frame_parse_loop0 with
  | .loop (.ite c _ _) _ => c
  | _ => .const .null

theorem loop0_eq : Gen.tg3__glb_frame_parse_loop0 = .loop (.ite loopCond loopBody .brk) .skip := rfl

theorem cond_eval (hdr off : Nat) (fr : Glb.Frame) (s : State)
    (hs : LoopFacts m bd bf d t hdr off fr s) (hoff : off ≤ d.length) :
    EvE Gen.prog s loopCond (boolVal (decide (hdr ≤ d.length - off))) := by
  have := hs.l1; have := hs.l6; have := hs.l7
  refine EvE.of_eq 5 (fun n => ?_)
  simp only [loopCond, Gen.tg3__glb_frame_parse_loop0]
  msimp

/-- Reading the chunk header (first statement of the body). -/
theorem header_exs (hdr off : Nat) (fr : Glb.Frame) (s : State)
    (hs : LoopFacts m bd bf d t hdr off fr s) (hh : hdr = 8 ∨ hdr = 12)
    (hge : hdr ≤ d.length - off) (hoff : off ≤ d.length) (hl : d.length < 18446744073709551616) :
    ∃ s1, ExS Gen.prog s (match loopBody with | .seq a _ => a | _ => .skip) .normal s1 ∧
      s1.mem = s.mem ∧ s1.ro = s.ro ∧
      (∀ i, i ≠ 4 → i ≠ 8 → s1.locals i = s.locals i) ∧
      s1.locals 8 = some (.int u64 (Glb.chunkLen d hdr off)) ∧
      s1.locals 4 = some (.int u32 (Glb.chunkType d hdr off)) := by
  have l0 := hs.l0; have l6 := hs.l6; have l7 := hs.l7; have mbd := hs.mbd
  have hv0 : EvE Gen.prog s (.var 0) (.ptr bd 0) := EvE.var l0
  have hv6 : EvE Gen.prog s (.var 6) (.int u64 off) := EvE.var l6
  simp only [loopBody, Gen.tg3__glb_frame_parse_loop0]
  rcases hh with rfl | rfl
  · -- GLB v2: 32-bit chunk length
    have hc : EvE Gen.prog s (.bin .eq (some ⟨64, false⟩) (.var 7)
        (.cast ⟨64, false⟩ (.const (.int ⟨32, false⟩ 12)))) (boolVal false) :=
      EvE.of_eq 5 (fun n => by msimp)
    have hr0 := rd32_call (s := s) mbd (o := off) (by omega) hl hv0 hv6
    have hcast : EvE Gen.prog s (.cast ⟨64, false⟩ (.call 0 [.var 0, .var 6]))
        (.int u64 (Glb.rd32 d off)) :=
      EvE.cast hr0 (by simp [evalCast, IntTy.modulus, IntTy.u32, IntTy.u64])
    let s1 : State := { s with locals := s.locals.upd 8 (.int u64 (Glb.rd32 d off)) }
    have hv0' : EvE Gen.prog s1 (.var 0) (.ptr bd 0) := EvE.var (by simp [s1, l0])
    have ha4 : EvE Gen.prog s1 (.bin .add (some ⟨64, false⟩) (.var 6)
        (.cast ⟨64, false⟩ (.const (.int ⟨32, false⟩ 4)))) (.int u64 (off + 4)) :=
      EvE.of_eq 5 (fun n => by msimp [s1, (by omega : off + 4 < 18446744073709551616)])
    refine ⟨_, ExS.ite hc rfl (ExS.seq (ExS.set hcast)
      (ExS.call_dst (EvArgs.cons hv0' (EvArgs.cons ha4 EvArgs.nil)) rfl rfl
        (rd32_exs s1.mem bd d (by simp [s1, mbd]) (off + 4) (by omega) hl s1.ro))), ?_⟩
    refine ⟨rfl, rfl, ?_, ?_, ?_⟩
    · intro i h4 h8; simp [Locals.upd, h4, h8]
    · simp [Locals.upd, Glb.chunkLen, evalCast, IntTy.modulus, rd32_lt]
    · simp [Locals.upd, Glb.chunkType]
  · -- GLB v3 (spec-unstable): 64-bit chunk length
    have hc : EvE Gen.prog s (.bin .eq (some ⟨64, false⟩) (.var 7)
        (.cast ⟨64, false⟩ (.const (.int ⟨32, false⟩ 12)))) (boolVal true) :=
      EvE.of_eq 5 (fun n => by msimp)
    let s1 : State := { s with locals := s.locals.upd 8 (.int u64 (Glb.rd64 d off)) }
    have hv0' : EvE Gen.prog s1 (.var 0) (.ptr bd 0) := EvE.var (by simp [s1, l0])
    have ha8 : EvE Gen.prog s1 (.bin .add (some ⟨64, false⟩) (.var 6)
        (.cast ⟨64, false⟩ (.const (.int ⟨32, false⟩ 8)))) (.int u64 (off + 8)) :=
      EvE.of_eq 5 (fun n => by msimp [s1, (by omega : off + 8 < 18446744073709551616)])
    refine ⟨_, ExS.ite hc rfl (ExS.seq
      (ExS.call_dst (EvArgs.cons hv0 (EvArgs.cons hv6 EvArgs.nil)) rfl rfl
        (rd64_exs s.mem bd d mbd off (by omega) hl s.ro))
      (ExS.call_dst (EvArgs.cons hv0' (EvArgs.cons ha8 EvArgs.nil)) rfl rfl
        (rd32_exs s1.mem bd d (by simp [s1, mbd]) (off + 8) (by omega) hl s1.ro))), ?_⟩
    refine ⟨rfl, rfl, ?_, ?_, ?_⟩
    · intro i h4 h8; simp [Locals.upd, h4, h8]
    · simp [Locals.upd, Glb.chunkLen]
    · simp [Locals.upd, Glb.chunkType]

/-- Everything in the body after reading the chunk header. -/
def restStmt : Stmt := match loopBody with | .seq _ b => b | _ => .skip

theorem body_eq : loopBody = .seq (match loopBody with | .seq a _ => a | _ => .skip) restStmt := rfl

structure MidFacts (bd bf : Nat) (d t : List UInt8) (hdr off clen ctype : Nat) (fr : Glb.Frame)
    (s : State) : Prop where
  l0 : s.locals 0 = some (.ptr bd 0)
  l1 : s.locals 1 = some (.int u64 d.length)
  l2 : s.locals 2 = some (.ptr bf 0)
  l4 : s.locals 4 = some (.int u32 ctype)
  l6 : s.locals 6 = some (.int u64 off)
  l7 : s.locals 7 = some (.int u64 hdr)
  l8 : s.locals 8 = some (.int u64 clen)
  mbd : s.mem bd = some d
  mbf : s.mem bf = some (frameBytes fr t)
  ro : s.ro = false

theorem rest_err (hdr off clen ctype : Nat) (fr : Glb.Frame) (s : State)
    (hs : MidFacts bd bf d t hdr off clen ctype fr s) (hne : bd ≠ bf) (ht : t.length = 4)
    (hge : off + hdr ≤ d.length) (hl : d.length < 18446744073709551616)
    (hclen : d.length - (off + hdr) < clen) :
    ExS Gen.prog s restStmt (.ret (some (.int u32 23)))
      { s with locals := s.locals.upd 6 (.int u64 (off + hdr)) } := by
  have := hs.l0; have := hs.l1; have := hs.l2; have := hs.l4; have := hs.l6; have := hs.l7
  have := hs.l8; have := hs.mbd; have := hs.mbf; have := hs.ro
  refine ExS.of_eq 20 (fun n => ?_)
  simp only [restStmt, loopBody, Gen.tg3__glb_frame_parse_loop0]
  msimp [(by omega : off + hdr < 18446744073709551616)]

set_option maxHeartbeats 2000000 in
theorem rest_ok (hdr off clen ctype : Nat) (fr : Glb.Frame) (s : State) (m : Mem)
    (hs : MidFacts bd bf d t hdr off clen ctype fr s) (hother : ∀ b, b ≠ bf → s.mem b = m b)
    (hne : bd ≠ bf) (ht : t.length = 4) (hge : off + hdr ≤ d.length)
    (hl : d.length < 18446744073709551616) (hclen : ¬ d.length - (off + hdr) < clen)
    (hct : ctype < 4294967296) :
    ∃ s', ExS Gen.prog s restStmt .normal s' ∧
      LoopFacts m bd bf d t hdr (Glb.next (off + hdr + clen) d.length)
        (Glb.record fr ctype (off + hdr) clen) s' := by
  have l0 := hs.l0; have l1 := hs.l1; have l2 := hs.l2; have l4 := hs.l4; have l6 := hs.l6
  have l7 := hs.l7; have l8 := hs.l8; have mbd := hs.mbd; have mbf := hs.mbf; have ro := hs.ro
  have hne' : bf ≠ bd := Ne.symm hne
  have e1 : off + hdr < 18446744073709551616 := by omega
  have e2 : off + hdr + clen < 18446744073709551616 := by omega
  have e3 : off + hdr + clen ≤ d.length := by omega
  simp only [restStmt, loopBody, Gen.tg3__glb_frame_parse_loop0]
  by_cases hpad : d.length - (off + hdr + clen) < (4 - (off + hdr + clen) % 4) % 4
  · by_cases hj : ctype = 1313821514
    · refine ⟨?sPJ, ExS.of_eq 30 (fun n => ?hPJ), ?fPJ⟩
      case hPJ => msimp; rfl
      case fPJ =>
        constructor <;> simp [Locals.upd, Glb.next, Glb.pad, Glb.record, Glb.chunkJSON, Glb.chunkBIN, frameBytes, b2n, Nat.min_def, l0, l1, l2, l7, mbd, mbf, hne', hother, u64, IntTy.u64, hj, hpad] <;> (try split) <;> (try simp_all) <;> omega
    · by_cases hb : ctype = 5130562
      · refine ⟨?sPB, ExS.of_eq 30 (fun n => ?hPB), ?fPB⟩
        case hPB => msimp; rfl
        case fPB =>
          constructor <;> simp [Locals.upd, Glb.next, Glb.pad, Glb.record, Glb.chunkJSON, Glb.chunkBIN, frameBytes, b2n, Nat.min_def, l0, l1, l2, l7, mbd, mbf, hne', hother, u64, IntTy.u64, hj, hb, hpad] <;> (try split) <;> (try simp_all) <;> omega
      · refine ⟨?sPO, ExS.of_eq 30 (fun n => ?hPO), ?fPO⟩
        case hPO => msimp; rfl
        case fPO =>
          constructor <;> simp [Locals.upd, Glb.next, Glb.pad, Glb.record, Glb.chunkJSON, Glb.chunkBIN, frameBytes, b2n, Nat.min_def, l0, l1, l2, l7, mbd, mbf, hne', hother, u64, IntTy.u64, hj, hb, hpad] <;> (try split) <;> (try simp_all) <;> omega
  · by_cases hj : ctype = 1313821514
    · refine ⟨?sQJ, ExS.of_eq 30 (fun n => ?hQJ), ?fQJ⟩
      case hQJ => msimp; rfl
      case fQJ =>
        constructor <;> simp [Locals.upd, Glb.next, Glb.pad, Glb.record, Glb.chunkJSON, Glb.chunkBIN, frameBytes, b2n, Nat.min_def, l0, l1, l2, l7, mbd, mbf, hne', hother, u64, IntTy.u64, hj, hpad] <;> (try split) <;> (try simp_all) <;> omega
    · by_cases hb : ctype = 5130562
      · refine ⟨?sQB, ExS.of_eq 30 (fun n => ?hQB), ?fQB⟩
        case hQB => msimp; rfl
        case fQB =>
          constructor <;> simp [Locals.upd, Glb.next, Glb.pad, Glb.record, Glb.chunkJSON, Glb.chunkBIN, frameBytes, b2n, Nat.min_def, l0, l1, l2, l7, mbd, mbf, hne', hother, u64, IntTy.u64, hj, hb, hpad] <;> (try split) <;> (try simp_all) <;> omega
      · refine ⟨?sQO, ExS.of_eq 30 (fun n => ?hQO), ?fQO⟩
        case hQO => msimp; rfl
        case fQO =>
          constructor <;> simp [Locals.upd, Glb.next, Glb.pad, Glb.record, Glb.chunkJSON, Glb.chunkBIN, frameBytes, b2n, Nat.min_def, l0, l1, l2, l7, mbd, mbf, hne', hother, u64, IntTy.u64, hj, hb, hpad] <;> (try split) <;> (try simp_all) <;> omega

theorem chunkType_lt (d : List UInt8) (hdr off : Nat) : Glb.chunkType d hdr off < 4294967296 := by
  unfold Glb.chunkType; split <;> exact rd32_lt _ _

theorem truthy_boolVal (b : Bool) : truthy (boolVal b) = .ok b := by
  cases b <;> rfl

/-- **The chunk loop refines `Glb.loop`.** -/
theorem loop_exs (m : Mem) (bd bf : Nat) (d t : List UInt8) (hne : bd ≠ bf)
    (hl : d.length < 18446744073709551616) (ht : t.length = 4) :
    ∀ (fuel off : Nat) (fr : Glb.Frame) (s : State) (hdr : Nat), (hdr = 8 ∨ hdr = 12) →
    LoopFacts m bd bf d t hdr off fr s → off ≤ d.length → d.length - off < fuel →
    ∃ o s', ExS Gen.prog s Gen.tg3__glb_frame_parse_loop0 o s' ∧ ExitFacts m bd bf d s' ∧
      ((o = .ret (some (.int u32 23)) ∧ Glb.loop d hdr fuel off fr = .error .chunk) ∨
       (o = .normal ∧ ∃ fr', s'.mem bf = some (frameBytes fr' t) ∧
          Glb.loop d hdr fuel off fr = (if fr'.hasJson then .ok fr' else .error .chunk))) := by
  intro fuel
  induction fuel with
  | zero => intro off fr s hdr _ _ _ h; omega
  | succ fuel ih =>
    intro off fr s hdr hh hs hoff hfuel
    have hpos : 0 < hdr := by omega
    have hc := cond_eval m bd bf d t hdr off fr s hs hoff
    rw [loop0_eq]
    by_cases hge : hdr ≤ d.length - off
    · -- one more chunk
      have hct := truthy_boolVal (decide (hdr ≤ d.length - off))
      simp only [hge, decide_true] at hc hct
      obtain ⟨s1, hA, hmem, hro, hloc, h8, h4⟩ := header_exs m bd bf d t hdr off fr s hs hh hge hoff hl
      have mid : MidFacts bd bf d t hdr off (Glb.chunkLen d hdr off) (Glb.chunkType d hdr off) fr s1 :=
        { l0 := by rw [hloc 0 (by decide) (by decide)]; exact hs.l0
          l1 := by rw [hloc 1 (by decide) (by decide)]; exact hs.l1
          l2 := by rw [hloc 2 (by decide) (by decide)]; exact hs.l2
          l4 := h4
          l6 := by rw [hloc 6 (by decide) (by decide)]; exact hs.l6
          l7 := by rw [hloc 7 (by decide) (by decide)]; exact hs.l7
          l8 := h8
          mbd := by rw [hmem]; exact hs.mbd
          mbf := by rw [hmem]; exact hs.mbf
          ro := by rw [hro]; exact hs.ro }
      have hother1 : ∀ b, b ≠ bf → s1.mem b = m b := by intro b hb; rw [hmem]; exact hs.other b hb
      by_cases hclen : d.length - (off + hdr) < Glb.chunkLen d hdr off
      · have hR := rest_err bd bf d t hdr off _ _ fr s1 mid hne ht (by omega) hl hclen
        refine ⟨_, _, ExS.loop_ret (ExS.ite hc hct (ExS.seq hA hR)), ?_, Or.inl ⟨rfl, ?_⟩⟩
        · exact { l2 := by simp [Locals.upd, mid.l2], mbd := mid.mbd, ro := mid.ro, other := hother1 }
        · simp only [Glb.loop, hge, ite_true]
          rw [ite_cond_eq_true _ _ (eq_true (by omega))]
      · obtain ⟨s2, hR, hF⟩ := rest_ok bd bf d t hdr off _ _ fr s1 m mid hother1 hne ht (by omega) hl
          hclen (chunkType_lt d hdr off)
        have hnext : Glb.next (off + hdr + Glb.chunkLen d hdr off) d.length ≤ d.length := by
          have := Glb.pad_le (off + hdr + Glb.chunkLen d hdr off) d.length
          simp only [Glb.next]; omega
        obtain ⟨o, s', hL, hE, hres⟩ := ih _ _ s2 hdr hh hF hnext (by simp only [Glb.next]; omega)
        refine ⟨o, s', ExS.loop_next (ExS.ite hc hct (ExS.seq hA hR)) (Or.inl rfl) ExS.skip hL, hE, ?_⟩
        have hstep : Glb.loop d hdr (fuel + 1) off fr =
            Glb.loop d hdr fuel (Glb.next (off + hdr + Glb.chunkLen d hdr off) d.length)
              (Glb.record fr (Glb.chunkType d hdr off) (off + hdr) (Glb.chunkLen d hdr off)) := by
          simp only [Glb.loop, hge, ite_true]
          rw [ite_cond_eq_false _ _ (eq_false (by omega))]
        rw [hstep]; exact hres
    · -- no room for another chunk header: leave the loop
      have hct := truthy_boolVal (decide (hdr ≤ d.length - off))
      simp only [hge, decide_false] at hc hct
      refine ⟨_, _, ExS.loop_brk (ExS.ite hc hct ExS.brk), ?_, Or.inr ⟨rfl, fr, hs.mbf, ?_⟩⟩
      · exact { l2 := hs.l2, mbd := hs.mbd, ro := hs.ro, other := hs.other }
      · simp only [Glb.loop, hge, ite_false]

end

end GlTF.C.Proofs
