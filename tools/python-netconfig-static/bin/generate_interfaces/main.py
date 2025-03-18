#!/usr/bin/env python

import interfaces

from generate_interfaces import build_parser, generate


def main():
    (opts, args) = build_parser().parse_args()

    if opts.debug:
        interfaces.DEBUG = True

    interfaces.output.write_all(opts.output, generate(opts))
