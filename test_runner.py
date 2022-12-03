#!/usr/bin/env python

import glob
import os
import subprocess

## Simple test runner.

# -- config -----------------------

# Absolute path pointing to your cloned git repo of https://github.com/KhronosGroup/glTF-Sample-Models
sample_model_dir = "/home/syoyo/work/glTF-Sample-Models"
base_model_dir = os.path.join(sample_model_dir, "2.0")

# Include `glTF-Draco` when you build `loader_example` with draco support.
kinds = [ "glTF", "glTF-Binary", "glTF-Embedded", "glTF-MaterialsCommon"]
# ---------------------------------

failed = []
success = []

def run(filename):

    print("Testing: " + filename)
    cmd = ["./loader_example", filename]
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (stdout, stderr) = p.communicate()
    except:
        print("Failed to execute: ", cmd)
        raise

    if p.returncode != 0:
        failed.append(filename)
        print(stdout)
        print(stderr)
    else:
        success.append(filename)


def test():

    for d in os.listdir(base_model_dir):
        p = os.path.join(base_model_dir, d)
        if os.path.isdir(p):
            for k in kinds:
                targetDir = os.path.join(p, k)
                g = glob.glob(targetDir + "/*.gltf") + glob.glob(targetDir + "/*.glb")
                for gltf in g:
                    run(gltf)


def main():

    test()

    print("Success : {0}".format(len(success)))
    print("Failed  : {0}".format(len(failed)))

    for fail in failed:
        print("FAIL: " + fail)

if __name__ == '__main__':
    main()
