#!/usr/bin/env python

import os
import subprocess
import sys
import time


def indent(lst, indent=' '*4):
    """Indent multiline string"""
    if isinstance(lst, str):
        lst = [lst]
    lines = []
    for text in lst:
        text = str(text)
        lines.extend(text.split('\n'))
    return '\n'.join([indent + line for line in lines])


def memoize(f):
    """Decrorator for memoization of function arguments"""
    class memodict(dict):
        def __init__(self, f):
            self.f = f

        def __call__(self, *args):
            return self[args]

        def __missing__(self, key):
            ret = self[key] = self.f(*key)
            return ret

    return memodict(f)


def red_text(text):
    """Wrap string as red text"""
    if sys.stdout.isatty():
        return red_text.COLOR_SEQ % 31 + str(text) + red_text.RESET_SEQ
    else:
        return str(text)
red_text.RESET_SEQ = "\033[0m"
red_text.COLOR_SEQ = "\033[1;%dm"


@memoize
def have_program(util_name):
    """Check if util found in PATH list"""
    fpath, _ = os.path.split(util_name)
    if fpath:
        if os.path.isfile(util_name) and os.access(util_name, os.X_OK):
            return True
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, util_name)
            if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
                return True
    return False


def run_command(args, shell=False):
    """Run arbitrary command and return stdout/stderr as strings"""
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=shell)
    (stdoutdata, stderrdata) = p.communicate()
    if p.returncode:
        raise Exception("Command <<%s>> returned %d\nStdout:%s" % (args, p.returncode, stderrdata))
    return stdoutdata, stderrdata


def timed_text(s, level='INFO'):
    return '[{}] [{:.3f}] {}'.format(level, time.time(), s)
