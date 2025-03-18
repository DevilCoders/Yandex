#!/usr/bin/env python

from sys import stdin, stdout, argv

def parse_line(line):
    strength, factor = line.split()
    return (factor.split('|') + ['_'])[:2] + [float(strength)]

def parse_line_transposed(line):
    return list(map(parse_line(line).__getitem__, (1, 0, 2)))

def aggr_to_str(total_strength, aggregations):
    return ', '.join(('%s: %d%%' % (aggregation, strength * 100 / total_strength))
        for (aggregation, strength) in aggregations)

def main(parse_line):
    fstr = {}
    fagr = {}
    for line in stdin:
        factor, aggregation, strength = parse_line(line)
        fstr[factor] = fstr.get(factor, 0) + strength
        fagr.setdefault(factor, []).append((aggregation, strength))
    max_factor_len = max(map(len, fstr.keys()))
    factor_format = '%s\t%-' + str(max_factor_len) + 's\t(%s)\n'
    for factor, strength in sorted(fstr.items(), lambda x, y: cmp(y[1], x[1])):
        stdout.write(factor_format % (strength, factor, aggr_to_str(strength, fagr[factor])))

if __name__ == '__main__':
    args = argv[1:]
    main(parse_line_transposed if '-t' in args else parse_line)

