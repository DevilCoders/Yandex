#!/usr/bin/env python

import os
import sys
import json
from argparse import ArgumentParser


def parse_cmd():
    parser = ArgumentParser(usage='usage: %(prog)s [options]', prog="PROG")

    parser.add_argument("-s", "--srcdir", dest="source_dir", type=str, required=True,
                        help="obligatory. Source directory for configs)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()
    return options


def check_required_source_files(file_path, local_path, node):
    if not isinstance(node, dict):
        return True

    success = True
    if 'CgiSearchPrefix' in node or 'CgiSnippetPrefix' in node:
        if 'Tier' not in node:
            print >> sys.stderr, 'Missing "Tier" section in %s in %s' % (file_path, '->'.join(local_path))
            success = False
        if 'PrimusList' not in node:
            print >> sys.stderr, 'Missing "PrimusList" section in %s in %s' % (file_path, '->'.join(local_path))
            success = False
    return success


def traversal_check(checkers, file_path, local_path, node):
    success = True
    for checker in checkers:
        checker_success = checker(path, local_path, node)
        success = success and checker_success
    if not isinstance(node, dict):
        return success
    for key, child in node.items():
        local_path.append(key)
        try:
            child_success = traversal_check(checkers, file_path, local_path, child)
            success = success and child_success
        finally:
            local_path.pop()
    return success


if __name__ == "__main__":
    options = parse_cmd()

    success = True
    for fname in os.listdir(options.source_dir):
        if os.path.splitext(fname)[1] != '.json':
            continue
        path = os.path.abspath(os.path.join(options.source_dir, fname))
        if os.path.isdir(path):
            continue
        root = json.loads(open(path).read())
        traversal_checkers = [check_required_source_files]
        file_success = traversal_check(traversal_checkers, path, [], root)
        success = success and file_success

    if not success:
        print 'Failed'
        exit(1)
    exit(0)
