#!/usr/bin/env python
# encoding: utf-8

from itertools import chain, count
from contextlib import nested
from sys import argv

PrefixFieldsNumber = 4

def load_factors(f):
    return (l.strip('," \n\r') for l in f)

def make_factor_to_index_map(f):
    return dict(zip(load_factors(f), count(PrefixFieldsNumber + 1)))

def merge_indexes_into_ranges(indexes):
    inditer = iter(indexes)
    first = inditer.next()
    last = first
    for next in inditer:
        if next != last + 1:
            yield (first, last)
            first = next
        last = next
    yield (first, last)

def get_factors_ranges(fall, fsel):
    fmap = make_factor_to_index_map(fall)
    indexes = map(fmap.__getitem__, load_factors(fsel))
    return merge_indexes_into_ranges(sorted(indexes))

def print_range(range):
    if range[0] == range[1]:
        return str(range[0])
    return '{0}-{1}'.format(*range)

def get_cut_cmd_line(ranges):
    allRanges = chain(((1, PrefixFieldsNumber),), ranges)
    return 'cut -f ' + ','.join(map(print_range, allRanges))

def main():
    with nested(file('all_factors.txt'), file('matrixnet.fn')) as (fall, fsel):
        ranges = get_factors_ranges(fall, fsel)
    print get_cut_cmd_line(ranges)

if __name__ == '__main__':
    if len(argv) == 1:
        main()
    else:
        print '''
        Мне нужно два файла: matrixnet.fn (из antirobot/daemon_lib)
        и all_factors.txt (это то что пишет "antirobot_daemon -F"),
        на выход я напечатаю командную строку для фильтрования
        обучающих данных для матрикснета.
        '''

