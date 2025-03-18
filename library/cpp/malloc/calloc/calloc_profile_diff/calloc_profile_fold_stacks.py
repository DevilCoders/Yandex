#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys


def main():
    METRIC_PREFIX = '# diff: '
    folded_stack = ''
    metric = 0
    is_function_line = True
    for line in sys.stdin:
        line = line.strip()
        if line.startswith(METRIC_PREFIX):
            if folded_stack:
                sys.stdout.write(folded_stack + ' ' + metric + '\n')
            folded_stack = ''
            metric = line[len(METRIC_PREFIX):].split()[0]
            is_function_line = True
        else:
            if line:
                if is_function_line:
                    if folded_stack:
                        folded_stack = ';' + folded_stack
                    folded_stack = line + folded_stack
                is_function_line = not is_function_line
    if folded_stack:
        sys.stdout.write(folded_stack + ' ' + metric + '\n')


if __name__ == '__main__':
    main()
