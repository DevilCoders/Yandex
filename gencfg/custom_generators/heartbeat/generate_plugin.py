#!/skynet/python/bin/python

import os
import sys

MYDIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(MYDIR)
sys.path.append(os.path.dirname(os.path.dirname(MYDIR)))

from argparse import ArgumentParser
import imp


def parse_cmd():
    parser = ArgumentParser(description="Generate instance usage agent from gencfg source code")
    parser.add_argument("-p", "--plugin", type=str, required=True,
                        help="Obligatory. Plugin to generate")
    parser.add_argument("-o", "--output-file", type=str, required=True,
                        help="Obligatory. Output file")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    pdir = os.path.join(MYDIR, options.plugin)
    if not os.path.exists(pdir):
        raise Exception("Can not find plugin %s: directory %s does not exists" % (options.plugin, pdir))


def main(options):
    module_fname = os.path.join(MYDIR, options.plugin, "genplugin.py")

    foo = imp.load_source("main", module_fname)

    with open(options.output_file, 'w') as f:
        f.write(foo.main())


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
