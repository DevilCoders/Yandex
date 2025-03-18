#!/usr/bin/python

import json
import sys


if __name__ == '__main__':
    try:
        est_file = sys.argv[1]
    except IndexError:
        print('Usage: ./color_killer.py est_file')
        exit(1)
    try:
        inf = open(est_file)
    except IOError as ex:
        print('No est_file: %s' % ex)
    outf = open('.'.join((est_file, 'new')), 'w')
    for line in inf:
        spl = line.strip().split('\t')
        json_val = json.loads(spl[2])
        for choice_id, choice in enumerate(json_val.get('choice', [])):
            json_val['choice'][choice_id] = choice.split(' ')[0]
        print >> outf, '\t'.join((spl[0], spl[1], json.dumps(json_val)))
    inf.close()
    outf.close()
