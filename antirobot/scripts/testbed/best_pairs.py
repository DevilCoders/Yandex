#!/usr/bin/env python

from sys import argv

factor_names_file = 'factor_names.txt'
factor_pairs_file = 'matrixnet.fpair'

def load_factor_names(fname):
    with file(fname) as f:
        return dict(line.split() for line in f)

def convert_pairs_file():
    factor_names = load_factor_names(factor_names_file)
    factor_name_len = max(len(factor_name) for factor_name in factor_names.values())
    str_format = '%-16s %-' + str(factor_name_len + 2) + 's %s'
    with file(factor_pairs_file) as f:
        for line in f:
            f1, f2, strength = line.split()
            print(str_format % (strength, factor_names[f1], factor_names[f2]))

def main():
    global factor_names_file, factor_pairs_file
    if len(argv) > 1 and argv[1] in ('--help', '-h', '-?'):
        print('Usage: best_pairs.py [factor_names.txt] [matrixnet.fpair]')
        return

    if len(argv) > 1:
        factor_names_file = argv[1]
    if len(argv) > 2:
        factor_pairs_file = argv[2]

    convert_pairs_file()

if __name__ == '__main__':
    main()

