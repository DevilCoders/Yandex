import sys

import six

from library.python.bstr.common import gen_entropy

if six.PY3:
    long = int


def main(args):
    a = long(args[0]) * 10

    while a > 0:
        sys.stdout.write(gen_entropy(100000))
        a -= 1
