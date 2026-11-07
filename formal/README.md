# tinygltf v3 — formal specification and verification (Lean 4)

This directory contains

1. a **Lean 4 specification** of what the tinygltf v3 C runtime must do
   (validation rules, GLB container framing),
2. **proofs about the specification** (e.g. a validated model can be indexed
   without going out of bounds),
3. **MiniC**, a small MIT-licensed operational semantics for the C subset used
   by the "verified core" of `tiny_gltf_v3.c`, a translator from the real C
   source into MiniC, and
4. **machine-checked proofs that the translated C code refines the
   specification** (no undefined behaviour, termination, functional
   correctness, frame).

Everything builds with `lake build`; there is no `sorry`, and every theorem
depends only on Lean's three standard axioms (`propext`, `Classical.choice`,
`Quot.sound`). No Mathlib, no `native_decide`, no `bv_decide`.

## Build and check

```sh
cd formal
lake build                                  # spec, tests, MiniC, all proofs (~3 min)
python3 tools/c2lean.py --check             # GlTF/C/Gen.lean matches tiny_gltf_v3.c
python3 tools/cdiff.py --n 1000             # compiled C == MiniC == spec on random GLBs
```

`tools/c2lean.py` needs `clang` (it reads `clang -Xclang -ast-dump=json`);
`tools/cdiff.py` additionally needs a C compiler with ASan/UBSan.

## Status

| Tier | C code | What is proven | Status |
|---|---|---|---|
| Spec | — | `Validate.validate` (all strictness levels), `Glb.parseFrame`, `Glb.layout` | done |
| Spec proofs | — | `validate_ok_iff_safe`, `validate_strict_ok_iff`, `accessor_element_in_buffer`, `attribute_resolves`, `scene_node_resolves`, `parseFrame_ok_in` (every reported chunk is inside the input) | done |
| T1 (parse) | `tg3__rd_u32le`, `tg3__rd_u64le`, `tg3__glb_frame_parse` | `glb_frame_parse_refines`: for **every** input, no UB, terminates, returns the code of `Glb.parseFrame`, writes only `*f`, and `*f` holds the specified frame | **done** |
| T1 (write) | `tg3__wr_u32le`, `tg3__wr_u64le` | **`wr_u32le_writes`** / **`wr_u64le_writes`**: for every offset and value the C writers store exactly `encodeLE 4 v` / `encodeLE 8 v` at `p + off`, touching nothing else (no UB, terminate). Supporting: `splice_append`, `splice_getElem?`, `read_prefix/suffix`, `store_int`, `stores4`, `encodeLE_add/mod/8_eq` | **done** |
| T1 (header) | `tg3__glb_align4`, `tg3__glb_write_magic`, `tg3__glb_write_header32`, `tg3__glb_write_header64`, `tg3__glb_write_headers` | **`align4_correct`** (`align4` = `Glb.align4`), **`write_magic_writes`** (`"glTF"` at offset 0), **`write_header32_no_bin`/`write_header32_bin`/`write_header64_no_bin`/`write_header64_bin`** (exact field splices for both `has_bin` branches), and **`write_headers32_writes`/`write_headers64_writes`** (magic + dispatcher) | **done** |
| T1 (layout) | `tg3__glb_layout` | **`glb_layout_correct`**: for every input, no UB, terminates, returns 1 iff `Glb.layout` is `some`, and on success leaves exactly the specified `tg3__glb_frame` (as `frameBytes`) and total size in `*total_out` | **done** |
| T2 | `tg3__spec_*_mask`, `tg3__validate_indices`, `tg3__validate_resources` | masks/verdict equal `Validate.*` | **not started**: the translator now emits `memcmp`/`strlen` (`Expr.memcmp`, `Expr.strlen`) and string literals from a read-only literal block, and all eleven mask functions plus `tg3__str_eq`/`tg3__str_eq_cstr` are translated and differentially tested; the per-mask refinement proofs remain |
| T3 | base64, data-URI, `tg3_component_size`, `tg3_num_components`, `tg3_accessor_byte_stride` | `GlTF/B64.lean` specifies the decoder; **`component_size_correct`**, **`num_components_correct`**, **`b64_decoded_len_correct`**, **`b64_decode_char_correct`** (including the `u8`→`i32` promotion) are proven | **in progress**: remaining `tg3__b64_decode_into`, `tg3__b64_decode`, data-URI parsing, `tg3_accessor_byte_stride` |
| T4 | JSON lexer + number parsing (`tinygltf_json_c.h`) | memory safety, termination, grammar conformance; **correctly rounded** `strtod` (decision recorded) | todo |
| T5 | JSON tree → `tg3_model` (arena) | memory safety under an allocator spec; result corresponds to a Lean `fromJson` | todo |
| T6 | `tg3_parse` / `tg3_parse_glb` | composition of T1–T5: parsing untrusted bytes never hits UB; `TG3_OK` ⇒ `WellFormedSafe` (and `WellFormedStrict` under `TG3_STRICT`) | todo |
| T7 | serializer | `parse (write m) ≈ m` | optional |

## Layout

| File | Contents |
|---|---|
| `GlTF/Basic.lean` | `F64` (IEEE-754 doubles as bit patterns, with C comparison semantics), byte strings, index helpers |
| `GlTF/Model.lean` | the glTF object model, field-for-field with the `tg3_*` structs |
| `GlTF/Validate.lean` | normative validation rules: `indicesOk`, `rangesOk` (memory safety) and `*Mask` (spec conformance), `validate`, `WellFormedSafe`, `WellFormedStrict` |
| `GlTF/Glb.lean` | GLB v2 / v3 (glTF 2.1, spec-unstable) framing: `parseFrame`, `layout`, `parseFrame_ok_in` |
| `GlTF/Safety.lean` | theorems about the validation spec |
| `GlTF/Tests.lean` | kernel-checked regression tests, incl. MiniC negative controls (out-of-bounds access is UB) |
| `GlTF/C/Semantics.lean` | MiniC: syntax, values, byte-addressed memory, executable big-step semantics with explicit UB |
| `GlTF/C/Gen.lean` | **generated** from `tiny_gltf_v3.c` by `tools/c2lean.py` — do not edit |
| `GlTF/C/Logic.lean`, `Hoare.lean` | proof rules (`EvE`, `ExS`, total-correctness triples), all derived from the semantics |
| `GlTF/C/Lemmas.lean` | arithmetic / byte-encoding lemmas for symbolic execution |
| `GlTF/C/Splice.lean` | byte-range rewrite algebra used by the writer (`splice*`, `read_*`, `store_int`) |
| `GlTF/B64.lean` | base64 decoder specification (`charValue`, `step`, `decode`, `decodedLen`) |
| `GlTF/C/Proofs/*.lean` | T1 proofs (`GlbParse`, `Loop`, `GlbMain`) |
| `CDiff.lean`, `tools/cdiff*.{py,c}` | three-way differential testing driver |

## What is trusted

A proof here is a statement about the MiniC semantics of the C source, so
the trusted base is:

* **the Lean kernel**;
* **the MiniC semantics** (`GlTF/C/Semantics.lean`, ~500 lines). Every
  operation that is undefined in C11 and can occur in the subset is `ub`:
  signed overflow, division by zero, oversized shifts, out-of-object loads,
  stores and pointer arithmetic, reading an uninitialised local. Memory is
  little-endian and byte addressed. Not modelled: strict aliasing, alignment,
  `volatile`, concurrency;
* **the translator** `tools/c2lean.py` and **clang's front end** (types and
  implicit conversions come from clang's AST; record layouts are measured by
  compiling a probe with the same compiler). The translator rejects anything
  outside the subset instead of guessing;
* **the target**: proofs assume an LP64, little-endian ABI (x86-64,
  AArch64, RISC-V 64 in practice) — the layout the probe measured;
* **the C compiler**: there is no link to compiled machine code.

The translator and semantics are validated by `tools/cdiff.py`, which runs
the compiled C (with ASan/UBSan), the MiniC interpreter on the translated
code, and the Lean specification on the same random and adversarial GLB
inputs and requires all three to agree.

## Spec ↔ C mapping

| Lean | C |
|---|---|
| `Validate.indicesOk` | `tg3__validate_indices` |
| `Validate.rangesOk` | `tg3__validate_resources` |
| `Validate.bufferMask`, …, `Validate.modelMask` | `tg3__spec_buffer_mask`, …, `tg3__spec_model_mask` (same bit numbers as the `TG3__R_*` macros) |
| `Validate.validate` | `tg3__validate_model` / `tg3_validate` |
| `Glb.parseFrame` | `tg3__glb_frame_parse` |
| `Glb.layout` | `tg3__glb_layout` |

When changing any of these C functions, update the Lean side, run
`tools/c2lean.py` and re-run `lake build`; the proofs will point at what
changed.

## Design notes

* Doubles are bit patterns (`F64`) with comparison defined on the bits
  (`F64.key`), so rules such as `0 <= alphaCutoff` are decidable and NaN
  fails every range check, exactly as in C.
* Loops in translated functions are hoisted into named `Stmt` definitions
  (`…_loopN`) so proofs can reason about them separately.
* String comparisons go through the internal pointer-based helpers
  `tg3__str_eq` / `tg3__str_eq_cstr` (the public by-value functions are thin
  wrappers). This keeps struct-by-value ABI modelling out of MiniC.
* Pointers stored in memory are modelled as the packed pair
  `(block, offset)` in 8 bytes (`ptrPack`/`ptrUnpack`); the verified core never
  round-trips a pointer through integer representation.
* `memcmp`/`strlen` are modelled by `Expr.memcmp`/`Expr.strlen` with their
  C-standard semantics (`memcmp` compares as `unsigned char`, `strlen` counts
  to the first NUL and is UB if there is none).
* Expression-level calls run in a read-only mode (stores are UB); the
  translator only emits them for functions that do not write memory, and the
  semantics enforces it.
