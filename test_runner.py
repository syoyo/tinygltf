#!/usr/bin/env python

import glob
import os
import re
import subprocess
import sys

## Cross-version verifier: parses each sample model with the mature v1
## (loader_example) and the new v3 C tester, then compares both the COUNTS
## summary line and the structured DIGEST block both binaries emit.
## v1 is the ground truth.

# -- config -----------------------

sample_model_dir = "/mnt/nfs/syoyo/glTF-Sample-Models"
base_model_dir = os.path.join(sample_model_dir, "2.0")

v1_bin = "./loader_example"
v3_bin = "./tests/tester_v3_c"

kinds = ["glTF", "glTF-Binary", "glTF-Embedded", "glTF-MaterialsCommon"]
# ---------------------------------

COUNTS_RE = re.compile(r"^COUNTS\s+(.*)$", re.MULTILINE)
DIGEST_RE = re.compile(r"^DIGEST_BEGIN\n(.*?)^DIGEST_END$", re.MULTILINE | re.DOTALL)


def parse_counts(output):
    m = COUNTS_RE.search(output)
    if not m:
        return None
    counts = {}
    for tok in m.group(1).split():
        if "=" not in tok:
            continue
        k, v = tok.split("=", 1)
        counts[k] = int(v)
    return counts


def parse_digest(output):
    m = DIGEST_RE.search(output)
    if not m:
        return None
    return [line for line in m.group(1).splitlines() if line]


def run_binary(binary, filename):
    p = subprocess.Popen(
        [binary, filename], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    out, err = p.communicate()
    return p.returncode, out.decode("utf-8", "replace"), err.decode("utf-8", "replace")


def diff_digests(v1, v3, max_lines=20):
    """Return a short summary of differences between two digest line lists."""
    diffs = []
    n = max(len(v1), len(v3))
    for i in range(n):
        a = v1[i] if i < len(v1) else "<missing>"
        b = v3[i] if i < len(v3) else "<missing>"
        if a != b:
            diffs.append("  v1[{0}]: {1}".format(i, a))
            diffs.append("  v3[{0}]: {1}".format(i, b))
            if len(diffs) >= max_lines * 2:
                diffs.append("  ... (truncated)")
                break
    return diffs


parse_failed = []     # v3 returned non-zero or no COUNTS/DIGEST
v1_skipped = []       # v1 returned non-zero or no COUNTS/DIGEST
counts_diff = []      # counts disagree
digest_diff = []      # digest disagrees
ok = []


def verify(filename):
    print("Testing: " + filename)

    rc1, out1, err1 = run_binary(v1_bin, filename)
    c1 = parse_counts(out1) if rc1 == 0 else None
    d1 = parse_digest(out1) if rc1 == 0 else None
    if c1 is None or d1 is None:
        v1_skipped.append(filename)
        print("  v1 ground truth unavailable (rc={0}); skipping".format(rc1))
        return

    rc3, out3, err3 = run_binary(v3_bin, filename)
    c3 = parse_counts(out3) if rc3 == 0 else None
    d3 = parse_digest(out3) if rc3 == 0 else None
    if c3 is None or d3 is None:
        parse_failed.append((filename, rc3, err3.strip()))
        print("  v3 FAILED (rc={0}): {1}".format(rc3, err3.strip()[:200]))
        return

    cdiffs = []
    for k in sorted(set(c1) | set(c3)):
        if c1.get(k) != c3.get(k):
            cdiffs.append((k, c1.get(k), c3.get(k)))
    if cdiffs:
        counts_diff.append((filename, cdiffs))
        print("  COUNTS MISMATCH:")
        for k, a, b in cdiffs:
            print("    {0}: v1={1} v3={2}".format(k, a, b))
        return

    if d1 != d3:
        diffs = diff_digests(d1, d3)
        digest_diff.append((filename, diffs))
        print("  DIGEST MISMATCH ({0} v1 lines, {1} v3 lines):".format(len(d1), len(d3)))
        for line in diffs[:8]:
            print(line)
        return

    ok.append(filename)


def test():
    for d in sorted(os.listdir(base_model_dir)):
        p = os.path.join(base_model_dir, d)
        if not os.path.isdir(p):
            continue
        for k in kinds:
            targetDir = os.path.join(p, k)
            g = sorted(
                glob.glob(targetDir + "/*.gltf")
                + glob.glob(targetDir + "/*.glb")
            )
            for gltf in g:
                verify(gltf)


def main():
    if not os.path.exists(v1_bin):
        sys.exit("error: v1 binary not found at {0}".format(v1_bin))
    if not os.path.exists(v3_bin):
        sys.exit("error: v3 binary not found at {0}".format(v3_bin))

    test()

    print("")
    print("=== Summary ===")
    print("OK            : {0}".format(len(ok)))
    print("Counts diff   : {0}".format(len(counts_diff)))
    print("Digest diff   : {0}".format(len(digest_diff)))
    print("v3 failed     : {0}".format(len(parse_failed)))
    print("v1 skipped    : {0}".format(len(v1_skipped)))

    for f, diffs in counts_diff:
        print("COUNTS DIFF: " + f)
        for k, a, b in diffs:
            print("  {0}: v1={1} v3={2}".format(k, a, b))
    for f, diffs in digest_diff:
        print("DIGEST DIFF: " + f)
        for line in diffs:
            print(line)
    for f, rc, err in parse_failed:
        print("V3 FAIL: {0} (rc={1}) {2}".format(f, rc, err[:200]))

    if counts_diff or digest_diff or parse_failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
