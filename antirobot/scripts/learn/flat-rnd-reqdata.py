#!/usr/bin/env python

import sys
import argparse

import arcadia

from antirobot.scripts.learn.make_learn_data import data_types


FIELDS = (
    (str, 'Raw', 'reqid'),
    (str, 'Raw', 'ip'),
    (str, 'Raw', 'uidStr'),
    (str, 'Raw', 'service'),
    (str, 'Raw', 'request'),
    (str, 'TweakFlags', 'numDocs'),
    (int, 'TweakFlags', 'haveSyntax'),
    (int, 'TweakFlags', 'haveRestr'),
    (int, 'TweakFlags', 'quotes'),
    (int, 'TweakFlags', 'cgiUrlRestr'),
    (str, 'NumRedir', 'numRedirs'),
    (str, 'NumRedir', 'numRemovals'),
    (int, 'Flags', 'wasXmlSearch'),
    (int, 'Flags', 'wasShow'),
    (int, 'Flags', 'wasImageShow'),
    (int, 'Flags', 'wasAttempt'),
    (int, 'Flags', 'wasSuccess'),
    (int, 'Flags', 'wasEnteredHiddenImage'),
    (int, 'Flags', 'isRobot'),
    )

FIELD_NAMES = [x[2] for x in FIELDS]
FIELDS_DICT = dict((x[2], x) for x in FIELDS) # => a dict of 'field_name': field_data


def ParseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('--title', action='store_true', dest='title', help='print field names')
    parser.add_argument('--fields', action='store', dest='fields', help='print specified fields only (coma separated)')

    return parser.parse_args()


def GetAttr(rnd, field):
    return getattr(getattr(rnd, field[1]), field[2])


def main():
    opts = ParseArgs()

    if opts.title:
        print '\t'.join((x[2] for x in FIELDS))

    fields = FIELD_NAMES
    if opts.fields:
        fields = opts.fields.split(',')

    for line in sys.stdin:
        rndReq = data_types.RndReqData.FromString(line.strip())

        items = []
        for field in fields:
            fld_data = FIELDS_DICT[field]
            typ = fld_data[0]
            items.append(str(typ(GetAttr(rndReq, fld_data))))

        print '\t'.join(items)


if __name__ == "__main__":
    main()
