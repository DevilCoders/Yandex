from __future__ import print_function

import sys
import importlib
import threading

threading.stack_size(4096 * 1000)

from library.python.bstr.common import run_verbose


def main(**settings):
    if len(sys.argv) < 2:
        print('usage:', sys.argv[0], '{push|pull} --help')
        sys.exit(0)

    def do():
        remap = {
            'push3': 'push',
            'pull3': 'pull',
        }

        op = sys.argv[1]

        importlib.import_module('library.python.bstr.' + remap.get(op, op)).main(sys.argv[2:], **settings)

    run_verbose(do, '-v' in sys.argv or '--verbose' in sys.argv)
