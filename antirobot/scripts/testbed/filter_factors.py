#!/usr/bin/env python

import sys

def parse_slices(slices_str):
    '''Converts string "1,3,4-20,17,33-104" to the list of ranges:
    [(1, 2), (3, 4), (4, 21), (17, 18), (33, 105)].'''
    ranges = (map(int, slice_str.split('-')) for slice_str in slices_str.split(','))
    return [(range[0], range[-1] + 1) for range in map(list, ranges)]

def sub_array(slices, array):
    return sum((array[slice[0]:slice[1]] for slice in slices), [])

prefix_len = 4
factor_repeats = 16

def extend_slices(slices, factors_number):
    addings = [prefix_len + i * factors_number for i in range(factor_repeats)]
    return [(0, prefix_len)] + sorted((slice[0] + adding, slice[1] + adding)
            for slice in slices for adding in addings)

def filter_fields(slices, fin, fout):
    all_slices = None
    for line in fin:
        fields = line.split()
        if all_slices is None:
            all_slices = extend_slices(slices, (len(fields) - prefix_len) // factor_repeats)
        fout.write('\t'.join(sub_array(all_slices, fields)) + '\n')

def main():
    global factor_repeats
    if len(sys.argv) == 1:
        print >> sys.stderr, 'Usage: filter_factors.py <factor_numbers> [aggregations_number = %d]\n' % factor_repeats
        return
    if len(sys.argv) > 2:
        factor_repeats = int(sys.argv[2])
    filter_fields(parse_slices(sys.argv[1]), sys.stdin, sys.stdout)

if __name__ == '__main__':
    main()

