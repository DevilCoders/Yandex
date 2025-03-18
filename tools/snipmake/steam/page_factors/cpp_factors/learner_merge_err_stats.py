# -*- coding: utf-8 -*-

import argparse


def parse_args():
    parser = argparse.ArgumentParser(
        description='Aggregate error-file statistics from "learner.py" (merge).',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-f', dest='err_file', default='err.txt',
                        help='-')

    return parser.parse_args()


def update_stat(sum_docs_stat, doc_err_stat):
    if doc_err_stat['guids'] == 0:
        return

    sum_docs_stat['docs_count'] += 1

    sum_docs_stat['abs_try_parent'] += doc_err_stat['try_parent']
    sum_docs_stat['abs_guids'] += doc_err_stat['guids']
    sum_docs_stat['abs_ok'] += doc_err_stat['ok']
    sum_docs_stat['abs_wrong_nodes'] += doc_err_stat['wrong_nodes']
    sum_docs_stat['abs_not_found'] += doc_err_stat['not_found']

    sum_docs_stat['rel_try_parent'] += doc_err_stat['try_parent'] * 1.0 / doc_err_stat['guids']
    sum_docs_stat['rel_guids'] += doc_err_stat['guids'] * 1.0 / doc_err_stat['guids']
    sum_docs_stat['rel_ok'] += doc_err_stat['ok'] * 1.0 / doc_err_stat['guids']
    sum_docs_stat['rel_wrong_nodes'] += doc_err_stat['wrong_nodes'] * 1.0 / doc_err_stat['guids']
    sum_docs_stat['rel_not_found'] += doc_err_stat['not_found'] * 1.0 / doc_err_stat['guids']


def get_err_stat(errs):
    for line in errs:
        line = line.strip()
        if line == 'INFO: START PHASE "merge"':
            break


    sum_docs_stat = {'docs_count': 0,
                     'abs_try_parent': 0, 'abs_guids': 0, 'abs_ok': 0,
                     'abs_wrong_nodes': 0, 'abs_not_found': 0,
                     'rel_try_parent': 0, 'rel_guids': 0, 'rel_ok': 0,
                     'rel_wrong_nodes': 0, 'rel_not_found': 0}
    corrupted = True
    doc_err_stat = {'try_parent': 0, 'guids': 0, 'ok': 0,
                    'wrong_nodes': 0, 'not_found': 0}
    for line in errs:
        line = line.strip()
        if line.startswith('WARNING: skip doc'):
            corrupted = True
        elif line.endswith("try parent's guid"):
            doc_err_stat['try_parent'] += 1
        elif line.startswith('INFO: selected guid'):
            doc_err_stat['guids'] += 1
        elif line.startswith('INFO: OK'):
            doc_err_stat['ok'] += 1
        elif line.startswith('WARNING: smth wrong with nodes'):
            doc_err_stat['wrong_nodes'] += 1
        elif line.startswith('WARNING: not found'):
            doc_err_stat['not_found'] += 1
        elif line.startswith('INFO: doc_id'):
            if not corrupted:
                update_stat(sum_docs_stat, doc_err_stat)
            corrupted = False
            doc_err_stat = {'try_parent': 0, 'guids': 0, 'ok': 0,
                            'wrong_nodes': 0, 'not_found': 0}
        elif line == 'INFO: START PHASE "annotate"':
            if not corrupted:
                update_stat(sum_docs_stat, doc_err_stat)
            break


    return sum_docs_stat

def print_stat(stat):
    def print_avg(name):
        print('%s: %f' % (name, stat[name] * 1.0 / stat['docs_count']))

    print('docs_count: %f' % stat['docs_count'])

    print_avg('abs_try_parent')
    print_avg('abs_guids')
    print_avg('abs_ok')
    print_avg('abs_wrong_nodes')
    print_avg('abs_not_found')

    print_avg('rel_try_parent')
    print_avg('rel_guids')
    print_avg('rel_ok')
    print_avg('rel_wrong_nodes')
    print_avg('rel_not_found')


def main():
    args = parse_args()
    with open(args.err_file, 'r') as errs:
        stat = get_err_stat(errs)
    print_stat(stat)


if __name__ == '__main__':
    main()
