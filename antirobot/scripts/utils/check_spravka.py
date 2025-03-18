#!/usr/bin/env python

import os
import imp

if __name__ == "__main__":
    hookSourceFile  = os.path.join('..', '..', '..', 'devtools', 'fleur', 'imports', 'installhook.py')
    imp.load_source('installhook', os.path.join(os.path.dirname(__file__), hookSourceFile))


import argparse
from antirobot.scripts.utils import spravka2


def ParseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('-k', '--keys', dest='keys', action='store', required=True, help='path to the file with keys')
    parser.add_argument('-d', '--domain', dest='domain', action='store', help='domain for the spravka')
    parser.add_argument('-u', '--useragent', dest='userAgent', action='store', help='user agent of the spravka')

    parser.add_argument('spravka', help='text of the spravka')

    return parser.parse_args()


def main():
    opts = ParseArgs()

    with open(opts.keys, "r") as inp:
        spravka2.init_keyring("\n".join(inp.readlines()))

    spr = spravka2.Spravka(opts.spravka, domain=opts.domain)


if __name__ == "__main__":
    main()
