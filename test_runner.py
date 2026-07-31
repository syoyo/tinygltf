#!/usr/bin/env python

import glob
import os
import re
import subprocess
import sys

## Model verifier for the tinygltf v3 C runtime: parses every sample model
## with the v3 C tester (tests/tester_v3_c) and validates that it exits
## successfully and emits a well-formed COUNTS summary and DIGEST block.

# -- config -----------------------

sample_model_dir = "/mnt/nfs/syoyo/glTF-Sample-Models"
base_model_dir = os.path.join(sample_model_dir, "2.0")

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


parse_failed = []  # v3 returned non-zero or no COUNTS/DIGEST
digest_malformed = []  # COUNTS/DIGEST present but structurally wrong
ok = []


def verify(filename):
    print("Testing: " + filename)

    rc, out, err = run_binary(v3_bin, filename)
    if rc != 0:
        parse_failed.append((filename, rc, err.strip()))
        print("  v3 FAILED (rc={0}): {1}".format(rc, err.strip()[:200]))
        return

    counts = parse_counts(out)
    digest = parse_digest(out)
    if counts is None or digest is None:
        digest_malformed.append((filename, counts is None, digest is None))
        print("  v3 COUNTS/DIGEST MISSING (counts={0}, digest={1})".format(
            counts is not None, digest is not None))
        return

    if "asset" not in counts:
        digest_malformed.append((filename, "no asset count", None))
        print("  v3 COUNTS missing asset key")
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
    if not os.path.exists(v3_bin):
        sys.exit("error: v3 binary not found at {0}".format(v3_bin))

    test()

    print("")
    print("=== Summary ===")
    print("OK              : {0}".format(len(ok)))
    print("Malformed output: {0}".format(len(digest_malformed)))
    print("v3 failed       : {0}".format(len(parse_failed)))

    for f, rc, err in parse_failed:
        print("V3 FAIL: {0} (rc={1}) {2}".format(f, rc, err[:200]))

    if digest_malformed or parse_failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
