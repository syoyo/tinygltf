#!/usr/bin/env python3
"""Translate selected functions of tiny_gltf_v3.c into the MiniC deep
embedding (formal/GlTF/C/Semantics.lean).

Input is clang's fully type-annotated AST (-ast-dump=json), so every implicit
conversion is explicit. Struct layouts (field offsets, sizeof) are obtained
by compiling and running a probe program with the same compiler/target, so
they are the layouts the real compiler uses.

Anything outside the supported subset raises an error: the translator is
part of the trusted base and must never guess.

Usage:
    python3 formal/tools/c2lean.py            # regenerate
    python3 formal/tools/c2lean.py --check    # fail if output is stale
"""
import json
import os
import re
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "tiny_gltf_v3.c")
OUT = os.path.join(ROOT, "formal", "GlTF", "C", "Gen.lean")
CLANG = os.environ.get("CLANG", "clang")
TARGET = "x86_64-linux-gnu"

# Functions forming the verified core, in dependency order.
FUNCTIONS = [
    "tg3__rd_u32le",
    "tg3__rd_u64le",
    "tg3__glb_frame_parse",
    "tg3__glb_layout",
    "tg3__wr_u32le",
    "tg3__wr_u64le",
    "tg3__glb_write_magic",
    "tg3__glb_write_header32",
    "tg3__glb_write_header64",
    "tg3__glb_write_headers",
    "tg3__glb_align4",
    # T3: small helpers
    "tg3_component_size",
    "tg3_num_components",
    "tg3_accessor_byte_stride",
    "tg3__b64_decode_char",
    "tg3__b64_decoded_len",
    "tg3__b64_decode_into",
    "tg3_is_data_uri",
    # T2: string comparison and spec masks
    "tg3__str_eq",
    "tg3__str_eq_cstr",
    "tg3__unit_interval",
    "tg3__spec_buffer_mask",
    "tg3__spec_buffer_view_mask",
    "tg3__spec_accessor_mask",
    "tg3__spec_mesh_mask",
    "tg3__spec_material_mask",
    "tg3__spec_camera_mask",
    "tg3__spec_sampler_mask",
    "tg3__spec_skin_mask",
    "tg3__spec_light_mask",
    "tg3__spec_animation_mask",
    "tg3__spec_model_mask",
]

# String literals live in one read-only block at this index.
LIT_BLOCK = 4096

INT_TYPES = {
    "unsigned char": (8, False), "signed char": (8, True), "char": (8, True),
    "unsigned short": (16, False), "short": (16, True),
    "unsigned int": (32, False), "int": (32, True),
    "unsigned long": (64, False), "long": (64, True),
    "unsigned long long": (64, False), "long long": (64, True),
    "_Bool": (8, False),
}


def const_int(node):
    """Integer value of a constant expression node."""
    if isinstance(node, dict):
        if "value" in node and node["kind"] in ("ConstantExpr", "IntegerLiteral",
                                                "CharacterLiteral", "UnaryOperator"):
            try:
                return int(node["value"])
            except (TypeError, ValueError):
                pass
        if node["kind"] == "UnaryOperator" and node.get("opcode") == "-":
            return -const_int(node["inner"][0])
        for v in node.get("inner", []) or []:
            try:
                return const_int(v)
            except Unsupported:
                pass
    raise Unsupported("cannot evaluate case label")


class Unsupported(Exception):
    pass


def die(msg, node=None):
    loc = ""
    if node is not None:
        r = node.get("range", {}).get("begin", {})
        loc = f" (line {r.get('line', '?')})"
    raise Unsupported(msg + loc)


def ctype(node_or_type):
    t = node_or_type.get("type", node_or_type) if isinstance(node_or_type, dict) else node_or_type
    return t.get("desugaredQualType", t.get("qualType"))


def strip_quals(t):
    """Removes cv-qualifiers (`const`/`volatile`), including pointer-level
    ones such as `char *const`, and normalises spacing."""
    t = re.sub(r"\b(const|volatile|restrict|__restrict)\b", " ", t)
    t = re.sub(r"\s*\*\s*", "*", t)
    return re.sub(r"\s+", " ", t).strip()


class Ctx:
    def __init__(self, enums, layout):
        self.enums = enums
        self.layout = layout
        self.literals = []          # list of bytes strings
        self.literal_offsets = {}   # bytes -> offset

    def literal(self, data):
        if data not in self.literal_offsets:
            self.literal_offsets[data] = sum(len(x) + 1 for x in self.literals)
            self.literals.append(data)
        return self.literal_offsets[data]

    def literal_bytes(self):
        out = []
        for x in self.literals:
            out.extend(x)
            out.append(0)
        return out


def int_ty(t, node=None, enums=None):
    t = strip_quals(t)
    if t in INT_TYPES:
        w, s = INT_TYPES[t]
        return w, s
    if t.startswith("enum ") or t.startswith("tg3_error_code"):
        return 32, False  # every enum used here has only non-negative values
    die(f"unsupported integer type '{t}'", node)


def lean_intty(w, s):
    return f"⟨{w}, {'true' if s else 'false'}⟩"


RECORDS = set()


def is_ptr(t):
    return strip_quals(t).endswith("*")


def is_struct(t):
    """True for struct/union types (not pointers to them)."""
    t = strip_quals(t)
    if t.endswith("*"):
        return False
    if t.startswith("struct ") or t.startswith("union "):
        return True
    return t in RECORDS


def scalar_ty(t, node=None):
    t = strip_quals(t)
    if t == "double":
        return ".f64"
    if t.endswith("*"):
        return ".ptr"
    w, s = int_ty(t, node)
    return f"(.int {lean_intty(w, s)})"


class FnTranslator:
    def __init__(self, ctx, fn, fids):
        self.ctx = ctx
        self.fn = fn
        self.fids = fids
        self.vars = {}  # decl name+id -> int
        self.names = []
        self.loops = []  # hoisted loop statements (name, lean term)

    def var_id(self, decl):
        key = decl.get("id") or decl["name"]
        if key not in self.vars:
            self.vars[key] = len(self.vars)
            self.names.append(decl["name"])
        return self.vars[key]

    def ref_var(self, ref, node):
        # referencedDecl has the declaration id; match by id
        key = ref.get("id")
        if key in self.vars:
            return self.vars[key]
        die(f"reference to unknown local '{ref.get('name')}'", node)

    # ---- lvalues -------------------------------------------------------
    def lvalue(self, n):
        k = n["kind"]
        if k == "ParenExpr":
            return self.lvalue(n["inner"][0])
        if k == "DeclRefExpr":
            ref = n["referencedDecl"]
            if ref["kind"] in ("VarDecl", "ParmVarDecl"):
                if is_struct(ctype(n)):
                    # lowered to a pointer to the caller's object
                    return ("mem", f"(.var {self.ref_var(ref, n)})", ctype(n))
                return ("var", self.ref_var(ref, n))
            die(f"unsupported lvalue ref kind {ref['kind']}", n)
        if k == "MemberExpr":
            base = n["inner"][0]
            off = self.member_offset(n)
            if n["isArrow"]:
                return ("mem", f"(.field {self.rvalue(base)} {off})", ctype(n))
            lv = self.lvalue(base)
            if lv[0] != "mem":
                die("member of non-memory struct", n)
            return ("mem", f"(.field {lv[1]} {off})", ctype(n))
        if k == "StringLiteral":
            # `"data:"[i]` / decayed literal used as an lvalue: address it
            return ("mem", self.string_literal(n), ctype(n))
        if k == "ArraySubscriptExpr":
            base, idx = n["inner"]
            elem = self.ctx.layout.sizeof(ctype(n), n)
            return ("mem", f"(.index {self.rvalue(base)} {self.rvalue(idx)} {elem})", ctype(n))
        if k == "UnaryOperator" and n["opcode"] == "*":
            return ("mem", self.rvalue(n["inner"][0]), ctype(n))
        die(f"unsupported lvalue {k}", n)

    def member_offset(self, n):
        base = n["inner"][0]
        bt = strip_quals(ctype(base))
        if n["isArrow"]:
            if not bt.endswith("*"):
                die("arrow on non-pointer", n)
            bt = strip_quals(bt[:-1])
        return self.ctx.layout.offsetof(bt, n["name"], n)

    # ---- rvalues -------------------------------------------------------
    def rvalue(self, n):
        k = n["kind"]
        if k == "ParenExpr":
            return self.rvalue(n["inner"][0])
        if k == "IntegerLiteral" or k == "CharacterLiteral":
            w, s = int_ty(ctype(n), n)
            v = int(n["value"]) % (1 << w)
            return f"(.const (.int {lean_intty(w, s)} {v}))"
        if k == "FloatingLiteral":
            if strip_quals(ctype(n)) != "double":
                die("non-double float literal", n)
            bits = struct.unpack("<Q", struct.pack("<d", float(n["value"])))[0]
            return f"(.const (.f64 {bits}))"
        if k == "DeclRefExpr":
            ref = n["referencedDecl"]
            if ref["kind"] == "EnumConstantDecl":
                v = self.ctx.enums[ref["id"]]
                w, s = int_ty(ctype(n), n)
                return f"(.const (.int {lean_intty(w, s)} {v % (1 << w)}))"
            die("DeclRefExpr used as rvalue without LValueToRValue", n)
        if k in ("ImplicitCastExpr", "CStyleCastExpr"):
            ck = n["castKind"]
            inner = n["inner"][0]
            if ck == "LValueToRValue":
                lv = self.lvalue(inner)
                if lv[0] == "var":
                    return f"(.var {lv[1]})"
                return f"(.load {scalar_ty(lv[2], n)} {lv[1]})"
            if ck == "IntegralCast":
                w, s = int_ty(ctype(n), n)
                return f"(.cast {lean_intty(w, s)} {self.rvalue(inner)})"
            if ck in ("NoOp", "BitCast"):
                return self.rvalue(inner)
            if ck == "ArrayToPointerDecay":
                if inner["kind"] == "StringLiteral":
                    return self.string_literal(inner)
                lv = self.lvalue(inner)
                if lv[0] != "mem":
                    die("decay of non-memory array", n)
                return lv[1]
            if ck == "FunctionToPointerDecay":
                die("function pointer", n)
            die(f"unsupported cast {ck}", n)
        if k == "BinaryOperator":
            op = n["opcode"]
            a, b = n["inner"]
            if op == "&&":
                return f"(.land {self.rvalue(a)} {self.rvalue(b)})"
            if op == "||":
                return f"(.lor {self.rvalue(a)} {self.rvalue(b)})"
            ops = {"+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
                   "<<": "shl", ">>": "shr", "&": "band", "|": "bor", "^": "bxor",
                   "<": "lt", "<=": "le", ">": "gt", ">=": "ge", "==": "eq", "!=": "ne"}
            if op not in ops:
                die(f"unsupported binary operator {op} in expression", n)
            if op in ("<", "<=", ">", ">=", "==", "!="):
                ot = strip_quals(ctype(a))
                if is_ptr(ot):
                    die("pointer comparison", n)
                t = "none" if ot == "double" else f"(some {lean_intty(*int_ty(ot, n))})"
            else:
                rt = strip_quals(ctype(n))
                if is_ptr(rt):
                    die("pointer arithmetic via +/-; use subscripts", n)
                t = f"(some {lean_intty(*int_ty(rt, n))})"
            return f"(.bin .{ops[op]} {t} {self.rvalue(a)} {self.rvalue(b)})"
        if k == "UnaryOperator":
            op = n["opcode"]
            a = n["inner"][0]
            if op == "!":
                return f"(.un .lnot {lean_intty(32, True)} {self.rvalue(a)})"
            if op == "-":
                return f"(.un .neg {lean_intty(*int_ty(ctype(n), n))} {self.rvalue(a)})"
            if op == "~":
                return f"(.un .bnot {lean_intty(*int_ty(ctype(n), n))} {self.rvalue(a)})"
            if op == "&":
                lv = self.lvalue(a)
                if lv[0] != "mem":
                    die("address of a local", n)
                return lv[1]
            die(f"unsupported unary operator {op} in expression", n)
        if k == "ConditionalOperator":
            c, a, b = n["inner"]
            return f"(.cond {self.rvalue(c)} {self.rvalue(a)} {self.rvalue(b)})"
        if k == "CallExpr":
            builtin = self.builtin_call(n)
            if builtin is not None:
                return builtin
            fid, args = self.call_parts(n)
            return f"(.call {fid} [{', '.join(args)}])"
        if k in ("MemberExpr", "ArraySubscriptExpr"):
            lv = self.lvalue(n)
            if lv[0] != "mem":
                die(f"{k} of a non-memory object", n)
            if is_struct(lv[2]):
                die("struct value used in an expression", n)
            return f"(.load {scalar_ty(lv[2], n)} {lv[1]})"
        if k == "StringLiteral":
            die("string literal used as a value", n)
        die(f"unsupported expression {k}", n)

    def string_literal(self, n):
        data = n["value"].encode("utf-8")
        off = self.ctx.literal(data)
        return f"(.const (.ptr {LIT_BLOCK} {off}))"

    def builtin_call(self, n):
        """Maps the C library calls the verified core uses onto MiniC forms."""
        callee = n["inner"][0]
        while callee["kind"] in ("ImplicitCastExpr", "ParenExpr"):
            callee = callee["inner"][0]
        if callee["kind"] != "DeclRefExpr":
            return None
        name = callee["referencedDecl"].get("name")
        args = [self.rvalue(x) for x in n["inner"][1:]]
        if name in ("memcmp", "__builtin_memcmp", "tg3__memcmp"):
            if len(args) != 3:
                die("memcmp arity", n)
            return f"(.memcmp {args[0]} {args[1]} {args[2]})"
        if name in ("strlen", "__builtin_strlen", "tg3__strlen"):
            if len(args) != 1:
                die("strlen arity", n)
            return f"(.strlen {args[0]})"
        return None

    def call_parts(self, n):
        callee = n["inner"][0]
        while callee["kind"] in ("ImplicitCastExpr", "ParenExpr"):
            callee = callee["inner"][0]
        if callee["kind"] != "DeclRefExpr" or callee["referencedDecl"]["kind"] != "FunctionDecl":
            die("indirect call", n)
        name = callee["referencedDecl"]["name"]
        if name not in self.fids:
            die(f"call to untranslated function {name}", n)
        return self.fids[name], [self.rvalue(x) for x in n["inner"][1:]]

    # ---- statements ----------------------------------------------------
    def assign(self, lv, rhs_lean, n):
        if lv[0] == "var":
            return f"(.set {lv[0+1]} {rhs_lean})"
        return f"(.store {scalar_ty(lv[2], n)} {lv[1]} {rhs_lean})"

    def stmt(self, n):
        if not n:
            return ".skip"
        k = n["kind"]
        if k == "CompoundStmt":
            return self.seq([self.stmt(x) for x in n.get("inner", [])])
        if k == "NullStmt":
            return ".skip"
        if k == "DeclStmt":
            out = []
            for d in n["inner"]:
                if d["kind"] != "VarDecl":
                    die("unsupported declaration", d)
                if d.get("storageClass") == "static":
                    die("static local", d)
                vid = self.var_id(d)
                if "inner" in d:
                    out.append(f"(.set {vid} {self.rvalue(d['inner'][0])})")
            return self.seq(out)
        if k == "ReturnStmt":
            if "inner" in n:
                return f"(.ret (some {self.rvalue(n['inner'][0])}))"
            return "(.ret none)"
        if k == "BreakStmt":
            return ".brk"
        if k == "ContinueStmt":
            return ".cont"
        if k == "IfStmt":
            inner = n["inner"]
            c = self.rvalue(inner[0])
            a = self.stmt(inner[1])
            b = self.stmt(inner[2]) if n.get("hasElse") else ".skip"
            return f"(.ite {c} {a} {b})"
        if k == "WhileStmt":
            c, body = n["inner"]
            return self.hoist(f"(.loop (.ite {self.rvalue(c)} {self.stmt(body)} .brk) .skip)")
        if k == "ForStmt":
            init, condvar, cond, inc, body = n["inner"]
            if condvar:
                die("for with condition variable", n)
            c = self.rvalue(cond) if cond else "(.const (.int ⟨32, true⟩ 1))"
            return self.seq([self.stmt(init) if init else ".skip",
                             self.hoist(f"(.loop (.ite {c} {self.stmt(body)} .brk) {self.stmt(inc) if inc else '.skip'})")])
        if k == "SwitchStmt":
            return self.switch(n)
        if k == "BinaryOperator" and n["opcode"] == "=":
            lhs, rhs = n["inner"]
            lv = self.lvalue(lhs)
            r = rhs
            while r["kind"] == "ParenExpr" or (r["kind"] == "ImplicitCastExpr" and r["castKind"] == "NoOp"):
                r = r["inner"][0]
            if r["kind"] == "CallExpr" and lv[0] == "var" and ctype(r) == ctype(lhs):
                fid, args = self.call_parts(r)
                return f"(.call (some {lv[1]}) {fid} [{', '.join(args)}])"
            return self.assign(lv, self.rvalue(rhs), n)
        if k == "CompoundAssignOperator":
            lhs, rhs = n["inner"]
            lv = self.lvalue(lhs)
            ops = {"+=": "add", "-=": "sub", "*=": "mul", "|=": "bor", "&=": "band",
                   "^=": "bxor", "<<=": "shl", ">>=": "shr"}
            op = ops.get(n["opcode"]) or die(f"unsupported {n['opcode']}", n)
            ct = int_ty(ctype(n["computeResultType"]), n)
            lt = int_ty(ctype(lhs), n)
            cur = f"(.var {lv[1]})" if lv[0] == "var" else f"(.load {scalar_ty(lv[2], n)} {lv[1]})"
            rhs_c = self.rvalue(rhs) if op in ("shl", "shr") else f"(.cast {lean_intty(*ct)} {self.rvalue(rhs)})"
            val = f"(.cast {lean_intty(*lt)} (.bin .{op} (some {lean_intty(*ct)}) (.cast {lean_intty(*ct)} {cur}) {rhs_c}))"
            return self.assign(lv, val, n)
        if k == "UnaryOperator" and n["opcode"] in ("++", "--"):
            lv = self.lvalue(n["inner"][0])
            w, s = int_ty(ctype(n), n)
            # integer promotion: types narrower than int are promoted to int
            pw, ps = (32, True) if w < 32 else (w, s)
            cur = f"(.var {lv[1]})" if lv[0] == "var" else f"(.load {scalar_ty(lv[2], n)} {lv[1]})"
            op = "add" if n["opcode"] == "++" else "sub"
            val = (f"(.cast {lean_intty(w, s)} (.bin .{op} (some {lean_intty(pw, ps)}) "
                   f"(.cast {lean_intty(pw, ps)} {cur}) (.const (.int {lean_intty(pw, ps)} 1))))")
            return self.assign(lv, val, n)
        if k == "CallExpr":
            fid, args = self.call_parts(n)
            return f"(.call none {fid} [{', '.join(args)}])"
        if k in ("CStyleCastExpr",) and n["castKind"] == "ToVoid":
            return ".skip"
        die(f"unsupported statement {k}", n)

    def hoist(self, term):
        """Loops become named definitions so proofs can treat them opaquely."""
        name = f"{self.fn['name']}_loop{len(self.loops)}"
        self.loops.append((name, term))
        return name

    @staticmethod
    def terminates(node):
        """True if control never falls out of `node` (last statement returns
        or breaks)."""
        while node is not None:
            k = node["kind"]
            if k in ("ReturnStmt", "BreakStmt"):
                return True
            if k == "CompoundStmt":
                inner = node.get("inner") or []
                node = inner[-1] if inner else None
                continue
            return False
        return False

    def switch(self, n):
        """`switch` with grouped case labels; supports fallthrough and
        `default`. Every case must terminate (return/break) or fall through."""
        if "cond" in n:
            cond, body = n["cond"], n["body"]
        else:
            cond, body = n["inner"][0], n["inner"][1]
        sel = self.rvalue(cond)
        st = lean_intty(*int_ty(ctype(cond), n))
        groups = []  # (label or None, [statements])

        def absorb(node):
            labels = []
            while True:
                subs = node.get("inner", [])
                if node["kind"] == "DefaultStmt":
                    labels.append(None)
                    rest = subs
                else:
                    if not subs:
                        die("case label without value", node)
                    labels.append(const_int(subs[0]))
                    rest = subs[1:]
                if (len(rest) == 1 and rest[0]["kind"] in ("CaseStmt", "DefaultStmt")):
                    node = rest[0]
                    continue
                return labels, rest

        for c in body.get("inner", []):
            if c["kind"] not in ("CaseStmt", "DefaultStmt"):
                die(f"unexpected {c['kind']} inside switch", c)
            labels, stmts = absorb(c)
            for lab in labels:
                groups.append((lab, stmts))
        if not groups:
            return ".skip"
        bodies = [self.seq([self.stmt(x) for x in stmts] or [".skip"]) for _, stmts in groups]
        fall = [None] * len(groups)
        nxt = ".skip"
        for idx in range(len(groups) - 1, -1, -1):
            _, stmts = groups[idx]
            term = bool(stmts) and self.terminates(stmts[-1])
            fall[idx] = bodies[idx] if term else self.seq([bodies[idx], nxt])
            nxt = fall[idx]
        default = ".skip"
        for idx, (label, _) in enumerate(groups):
            if label is None:
                default = fall[idx]
        chain = default
        for idx in range(len(groups) - 1, -1, -1):
            label, _ = groups[idx]
            if label is None:
                continue
            chain = (f"(.ite (.bin .eq (some {st}) {sel} (.const (.int {st} {label}))) "
                     f"{fall[idx]} {chain})")
        return chain

    def seq(self, items):
        items = [x for x in items if x != ".skip"]
        if not items:
            return ".skip"
        out = items[-1]
        for x in reversed(items[:-1]):
            out = f"(.seq {x} {out})"
        return out

    def translate(self):
        params = [x for x in self.fn["inner"] if x["kind"] == "ParmVarDecl"]
        pids = [self.var_id(p) for p in params]
        body = [x for x in self.fn["inner"] if x["kind"] == "CompoundStmt"][0]
        b = self.stmt(body)
        return pids, b


class Layout:
    """Field offsets and sizes, measured with the real compiler."""

    def __init__(self):
        self.queries = {}
        self.results = None

    def _q(self, expr):
        if self.results is not None:
            if expr not in self.results:
                die(f"layout query {expr} missing after probing")
            return self.results[expr]
        self.queries[expr] = None
        return f"@@{expr}@@"

    def offsetof(self, rec, field, node=None):
        return self._q(f"offsetof({rec}, {field})")

    def sizeof(self, t, node=None):
        return self._q(f"sizeof({t})")

    def probe(self):
        with tempfile.TemporaryDirectory() as d:
            c = os.path.join(d, "probe.c")
            exe = os.path.join(d, "probe")
            with open(c, "w") as f:
                f.write(f'#include "{SRC}"\n#include <stddef.h>\n#include <stdio.h>\n')
                f.write("int main(void) {\n")
                for q in self.queries:
                    f.write(f'    printf("%zu\\n", (size_t){q});\n')
                f.write("    return 0;\n}\n")
            subprocess.check_call([CLANG, f"--target={TARGET}", "-std=c11", "-w",
                                   f"-I{ROOT}", "-o", exe, c])
            vals = subprocess.check_output([exe]).decode().split()
        self.results = dict(zip(self.queries, (int(v) for v in vals)))


def load_ast():
    out = subprocess.check_output(
        [CLANG, "-Xclang", "-ast-dump=json", "-fsyntax-only", "-std=c11",
         f"--target={TARGET}", f"-I{ROOT}", SRC], stderr=subprocess.DEVNULL)
    return json.loads(out)


def collect_enums(ast):
    enums = {}

    def walk(n):
        if isinstance(n, dict):
            if n.get("kind") == "EnumConstantDecl":
                ce = n["inner"][0]
                enums[n["id"]] = int(ce["value"])
            for v in n.get("inner", []):
                walk(v)
    walk(ast)
    return enums


def collect_records(ast):
    """Names of struct/union types (tags and typedefs of records)."""
    def walk(n):
        if isinstance(n, dict):
            k = n.get("kind")
            if k in ("RecordDecl", "CXXRecordDecl") and n.get("name"):
                RECORDS.add(n["name"])
            if k == "TypedefDecl" and n.get("name"):
                inner = n.get("inner") or []
                t = inner[0].get("type", {}).get("qualType", "") if inner else ""
                if t.startswith("struct ") or t.startswith("union "):
                    RECORDS.add(n["name"])
                elif t in RECORDS:
                    RECORDS.add(n["name"])
            for v in n.get("inner", []):
                walk(v)
    walk(ast)


def generate():
    ast = load_ast()
    collect_records(ast)
    enums = collect_enums(ast)
    fns = {}
    for n in ast["inner"]:
        if n.get("kind") == "FunctionDecl" and n.get("name") in FUNCTIONS and "inner" in n:
            if any(x["kind"] == "CompoundStmt" for x in n["inner"]):
                fns[n["name"]] = n
    missing = [f for f in FUNCTIONS if f not in fns]
    if missing:
        die(f"functions not found: {missing}")
    fids = {name: i for i, name in enumerate(FUNCTIONS)}
    layout = Layout()
    ctx = Ctx(enums, layout)
    # Two passes: collect layout queries, probe, then emit with real numbers.
    for _ in range(2):
        out = []
        for name in FUNCTIONS:
            tr = FnTranslator(ctx, fns[name], fids)
            pids, body = tr.translate()
            out.append((name, pids, body, tr.names, tr.loops))
        if layout.results is None:
            layout.probe()
    lines = [
        "/-",
        "  GENERATED by formal/tools/c2lean.py from tiny_gltf_v3.c — DO NOT EDIT.",
        f"  Target layout: {TARGET} (LP64, little-endian).",
        "-/",
        "import GlTF.C.Semantics",
        "",
        "namespace GlTF.C.Gen",
        "open GlTF.C",
        "",
    ]
    for i, (name, pids, body, names, loops) in enumerate(out):
        for lname, lterm in loops:
            lines.append(f"/-- A loop of `{name}`. -/")
            lines.append(f"def {lname} : Stmt := {lterm}")
            lines.append("")
        lines.append(f"/-- `{name}` (function id {i}). Locals: "
                     + ", ".join(f"{j}={nm}" for j, nm in enumerate(names)) + " -/")
        lines.append(f"def fid_{name} : Nat := {i}")
        lines.append(f"def {name} : Func where")
        lines.append(f"  params := {pids}")
        lines.append(f"  body := {body}")
        lines.append("")
    lit = ctx.literal_bytes()
    lines.append("/-- String literals referenced by the translated functions. -/")
    lines.append("def literals : List UInt8 := " + repr(lit).replace("'", '"'))
    lines.append("")
    lines.append("def prog : Prog := fun f =>")
    lines.append("  match f with")
    for i, (name, _, _, _, _) in enumerate(out):
        lines.append(f"  | {i} => some {name}")
    lines.append("  | _ => none")
    lines.append("")
    lines.append("end GlTF.C.Gen")
    return "\n".join(lines) + "\n"


def main():
    try:
        text = generate()
    except Unsupported as e:
        print(f"c2lean: unsupported construct: {e}", file=sys.stderr)
        return 2
    if "--check" in sys.argv:
        with open(OUT) as f:
            if f.read() != text:
                print("c2lean: formal/GlTF/C/Gen.lean is stale; run formal/tools/c2lean.py",
                      file=sys.stderr)
                return 1
        return 0
    with open(OUT, "w") as f:
        f.write(text)
    print("wrote", OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
