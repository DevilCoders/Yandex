#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
import argparse
import json


def getArgs():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-i', '--input', help='Input', required=True)
    return parser.parse_args()


def main():
    cfg = getArgs()

    with open(cfg.input) as _i:
        js = json.load(_i)
        for element in filter(lambda j: j[5] == "web", js):
            print(element[3])
            print(element[11])
        # print(json.dumps(js, ensure_ascii=False, indent=4))


if __name__ == "__main__":
    main()
