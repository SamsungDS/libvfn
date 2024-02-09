#! /usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0

# Invoke sparse based on the contents of compile_commands.json,
# also working around several deficiencies in cgcc's command line
# parsing
#
# By Paolo Bonzini, imported from QEMU source tree.

import json
import os
import re
import shlex
import subprocess
import sys

def cmdline_for_sparse(sparse, cmdline):
    # Do not include the C compiler executable
    skip = True
    arg = False
    out = sparse + ['-no-compile']
    for x in cmdline:
        if arg:
            out.append(x)
            arg = False
            continue
        if skip:
            skip = False
            continue
        # prevent sparse from treating output files as inputs
        if x == '-MF' or x == '-MQ' or x == '-o':
            skip = True
            continue
        # cgcc ignores -no-compile if it sees -M or -MM?
        if x.startswith('-M'):
            continue
        # sparse does not understand these!
        if x == '-iquote' or x == '-isystem':
            x = '-I'
        if x == '-I':
            arg = True
        out.append(x)
    return out

root_path = os.getenv('MESON_BUILD_ROOT')
def build_path(s):
    return s if not root_path else os.path.join(root_path, s)

ccjson_path = build_path(sys.argv[1])
with open(ccjson_path, 'r') as fd:
    def exclude_directories(command):
        dirs = ['../ccan/', '../subprojects/', 'subprojects/']
        for d in dirs:
            if command['file'].startswith(d):
                return False

        return True

    compile_commands = filter(exclude_directories, json.load(fd))

sparse = sys.argv[2:]
sparse_env = os.environ.copy()
found_warning = False
for cmd in compile_commands:
    cmdline = shlex.split(cmd['command'])
    cmd = cmdline_for_sparse(sparse, cmdline)
    #print('REAL_CC=%s' % shlex.quote(cmdline[0]), ' '.join((shlex.quote(x) for x in cmd)))
    sparse_env['REAL_CC'] = cmdline[0]
    r = subprocess.run(cmd, env=sparse_env, cwd=root_path, capture_output=True)
    if r.stderr:
        print(r.stderr.decode('utf-8'), end='')
    if re.search(b': warning:', r.stderr):
        found_warning = True
    if r.returncode != 0:
        sys.exit(r.returncode)

if found_warning:
    sys.exit(1)
