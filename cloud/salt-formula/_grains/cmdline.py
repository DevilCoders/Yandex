#!/usr/bin/env python
CMDLINE_FILE="/proc/cmdline"

def _cmp_line_parser():
    cmdline_grains = { }
    with open("/proc/cmdline", "r") as cmdline_content:
         #print([param.split('=', 1) for param in cmdline_content.read().strip().split(' ') if "=" in param])
         return dict(param.split('=', 1) if "=" in param else (param, param) for param in cmdline_content.read().strip().split(' '))


def main():
    # initialize a grains dictionary
    grains = {}
    grains['cmdline'] = _cmp_line_parser()
    return grains
