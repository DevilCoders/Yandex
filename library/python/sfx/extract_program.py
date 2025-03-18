"""
Extract source code from an Arcadia Python program.
"""

import inspect
import json
import os
import sys
from subprocess import Popen, PIPE, STDOUT, CalledProcessError

import six

from . import extract

template = '''
import sys
exec(sys.stdin.read())
{}
main()
'''


def extract_program_sources(program, out_dir, sym_source_root=None, keep_src_path=False):
    cmd = [program, out_dir, os.path.expanduser(sym_source_root or ''), '1' if keep_src_path else '']
    env = os.environ.copy()
    env['Y_PYTHON_ENTRY_POINT'] = 'code:interact'
    proc = Popen(cmd, env=env, stdin=PIPE, stdout=PIPE, stderr=STDOUT)
    out, _ = proc.communicate(six.ensure_binary(template.format(inspect.getsource(extract))))

    sys.stderr.write(six.ensure_str(out))
    if proc.returncode != 0:
        raise CalledProcessError(proc.returncode, cmd)

    with open(os.path.join(out_dir, 'source_map.json')) as f:
        return json.load(f)
