#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

import sys
import re
import hashlib
from argparse import ArgumentParser

import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Script to replace all <old name> to <new name> in configs")

    parser.add_argument("-o", "--old", dest="old", type=str, required=True,
                        help="Obligatory. Name to be replaced")
    parser.add_argument("-n", "--new", dest="new", type=str, required=True,
                        help="Obligatory. Replacing name")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on files to replace")
    parser.add_argument("-d", "--directory-or-file", dest="directory_or_file", type=argparse_types.comma_list,
                        required=True,
                        help="Obligatory. Directory/filename to process")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def _replace(fname, old, new):
    data = open(fname).read()

    md5sum = hashlib.sha224(data).hexdigest()
    data = re.sub(r"(\W)(%s)(\W)" % re.escape(old), r"\g<1>%s\g<3>" % new, data)
    data = re.sub(r"^(%s)(\W)" % re.escape(old), r"%s\g<2>" % new, data)
    data = re.sub(r"(\W)(%s)$" % re.escape(old), r"\g<1>%s" % new, data)
    if hashlib.sha224(data).hexdigest() != md5sum:
        print "File %s: replaced some instances of <%s> by <%s>" % (fname, old, new)

    f = open(fname, 'w')
    f.write(data)
    f.close()


def main(options):
    for path in options.directory_or_file:
        if os.path.isfile(path):
            if options.filter(path):
                _replace(path, options.old, options.new)
        else:
            for dirpath, dirnames, filenames in os.walk(path):
                for fname in filenames:
                    if options.filter(fname):
                        _replace(os.path.join(dirpath, fname), options.old, options.new)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
