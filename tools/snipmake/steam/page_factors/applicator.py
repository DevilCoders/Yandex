# -*- coding: utf8 -*-

import json
import os
import random
import traceback
import subprocess
import sys


PHANTOMJS_ARGS = ['nice', 'phantomjs', '--ignore-ssl-errors=true', 'extractor.js']
SPLIT_DIR = 'split'
SPLIT_FILENAME = 'apply_split.%d.txt'
SPLIT_FILEPATH = os.path.join(SPLIT_DIR, SPLIT_FILENAME)
MERGE_DIR = 'merge'
MERGE_FILENAME = 'apply_merge.%d.txt'
MERGE_FILEPATH = os.path.join(MERGE_DIR, MERGE_FILENAME)
ANNOT_DIR = 'annotate'
ANNOT_FILENAME = 'apply_annotation.%d.txt'
ANNOT_FILEPATH = os.path.join(ANNOT_DIR, ANNOT_FILENAME)
MXNET_SUFFIX = '.matrixnet'
ESTS_FILENAME = 'estimations2.txt'
CUSTOM_ESTS_FILENAME = os.path.join('ests', 'estimations.%d.txt')
CUSTOM_ESTS_RES_FILENAME = os.path.join('ests', 'estimations.%d.res.txt')
ESTS_START = 0
ESTS_END = 1
if len(sys.argv) > 2:
    ESTS_START = int(sys.argv[1])
    ESTS_END = int(sys.argv[2])
STEAM_USER = 'robot-steam-as'


def phantom_cmd(command, est_idx, ests_file=ESTS_FILENAME):
    return PHANTOMJS_ARGS + [command, 'apply', '-f', ests_file, str(est_idx), str(est_idx + 1)]

def split(est_idx):
    with open(SPLIT_FILEPATH % est_idx, 'w') as out:
        print('start split %d' % est_idx)
        p = subprocess.Popen(phantom_cmd('split', est_idx),
                             stdout=out)
        p.communicate()
        print('finish split %d' % est_idx)
    if p.returncode:
        sys.exit('split returns %d' % p.returncode)
    os.chdir(SPLIT_DIR)
    p = subprocess.Popen(
        ('nice', '../matrixnet', '-A', '-f', SPLIT_FILENAME % est_idx)
    )
    p.communicate()
    if p.returncode:
        sys.exit('matrixnet returns %d' % p.returncode)
    os.chdir('..')


def get_tree(est_idx):
    print('start tree %d' % est_idx)
    p = subprocess.Popen(phantom_cmd('tree', est_idx),
                         stdout=subprocess.PIPE)
    tree_out, tree_err = p.communicate()
    print('finish tree %d' % est_idx)
    if p.returncode:
        sys.exit('merge (get_tree) returns %d: %s' % (p.returncode, tree_out))
    tree = [[line.split(':')[0], line.split(':')[1].split('\t')]
            for line in tree_out.strip().split('\n')]
    return tree


def filter_tree(est_idx, tree):
    def split_ans(coef):
        return 1 if coef > 0 else 0

    def del_node(tree_dict, guid):
        if guid in tree_dict:
            childs = tree_dict[guid]
            del tree_dict[guid]
            for child in childs:
                del_node(tree_dict, child)

    split_res_file = open(SPLIT_FILEPATH % est_idx + MXNET_SUFFIX, 'r')
    split_res = {line.split()[0]: split_ans(float(line.split()[4]))
                 for line in split_res_file}
    split_res_file.close()
    tree_dict = dict(tree)
    for guid in split_res:
        if split_res[guid] == 0:
            del_node(tree_dict, guid)
    return [node for node in tree if node[0] in tree_dict]


def adjacent_nodes(tree):
    def nodes_seq(tree_dict, cur_node):
        for node in tree_dict[cur_node]:
            if node in tree_dict:
                for got_node in nodes_seq(tree_dict, node):
                    yield got_node
            else:
                yield node

    tree_dict = dict(tree)
    prev_node = None
    for node in nodes_seq(tree_dict, tree[0][0]):
        if prev_node is not None:
            yield prev_node, node
        prev_node = node


def get_steam_id(est_idx):
    with open(ESTS_FILENAME, 'r') as ests_file:
        for i, line in enumerate(ests_file):
            if i == est_idx:
                break
    return int(line.split('\t', 1)[0])


def create_ests_file(est_idx, est_steam_id, node_pairs):
    def merge_ans(coef):
        return 1 if coef > 0 else 0

    est = {'merge': [], 'choice': []}
    for i, node_pair in enumerate(node_pairs, start=1):
        if i == 1:
            est['merge'].append('%d,%d,AUX' % (0, 1))
            est['choice'].append(node_pair[0])
        est['merge'].append('%d,%d,AUX' % (i, i + 1))
        est['choice'].append(node_pair[1])
    ests_file = open(CUSTOM_ESTS_FILENAME % est_idx, 'w')
    ests_file.write('%d\t%s\t' % (est_steam_id, STEAM_USER))
    ests_file.write(json.dumps(est))
    ests_file.close()

    with open(MERGE_FILEPATH % est_idx, 'w') as out:
        print('start merge %d' % est_idx)
        p = subprocess.Popen(
            phantom_cmd('merge', 0, CUSTOM_ESTS_FILENAME % est_idx),
            stdout=out)
        p.communicate()
        print('finish merge %d' % est_idx)
    if p.returncode:
        sys.exit('merge (create_ests_file) returns %d' % p.returncode)

    merge_count = len(est['merge']) - 1
    merge_res = []
    prev_merge_count = 0
    for merge_idx in range(merge_count):
        os.chdir(MERGE_DIR)
        print('start mxnet_merge %d.%d' % (est_idx, merge_idx))
        p = subprocess.Popen(
            ('nice', '../matrixnet', '-A', '-f', MERGE_FILENAME % est_idx)
        )
        p.communicate()
        print('finish mxnet_merge %d.%d' % (est_idx, merge_idx))
        if p.returncode:
            sys.exit('matrixnet returns %d' % p.returncode)
        os.chdir('..')

        merge_res_file = open(MERGE_FILEPATH % est_idx + MXNET_SUFFIX, 'r')
        for i, line in enumerate(merge_res_file):
            if i == merge_idx:
                break
        merge_res.append(merge_ans(float(line.split()[4])))
        merge_res_file.close()

        if merge_res[-1] == 1:
            prev_merge_count += 1
        else:
            prev_merge_count = 0

        next_merge_idx = merge_idx + 1
        if next_merge_idx < merge_count:
            with open(MERGE_FILEPATH % est_idx, 'r') as merge_file:
                merge_file_lines = merge_file.readlines()
            splitted_factors = merge_file_lines[next_merge_idx].split()
            splitted_factors = splitted_factors[:-1] + [str(prev_merge_count)]
            merge_file_lines[next_merge_idx] = '\t'.join(splitted_factors)
            if next_merge_idx < merge_count - 1:
                merge_file_lines[next_merge_idx] += '\n'
            with open(MERGE_FILEPATH % est_idx, 'w') as merge_file:
                for line in merge_file_lines:
                    merge_file.write(line)

    est['merge'] = []
    merge_start = 0
    merge_end = 1
    for cur_merge_res in merge_res:
        if cur_merge_res == 1:
            merge_end += 1
        else:
            est['merge'].append('%d,%d,AUX' % (merge_start, merge_end))
            merge_start = merge_end
            merge_end += 1
    est['merge'].append('%d,%d,AUX' % (merge_start, merge_end))

    ests_file = open(CUSTOM_ESTS_FILENAME % est_idx, 'w')
    ests_file.write('%d\t%s\t' % (est_steam_id, STEAM_USER))
    ests_file.write(json.dumps(est))
    ests_file.close()


def merge(est_idx):
    tree = get_tree(est_idx)
    tree = filter_tree(est_idx, tree)
    if not tree:
        print('nothing to merge')
        # TODO
        return
    create_ests_file(est_idx, get_steam_id(est_idx), adjacent_nodes(tree))


def apply_labels(est_idx):
    print('start labels %d' % est_idx)
    p = subprocess.Popen(phantom_cmd('labels', 0, CUSTOM_ESTS_FILENAME % est_idx),
                         stdout=subprocess.PIPE)
    (out, err) = p.communicate()
    print('finish labels %d' % est_idx)
    if p.returncode:
        sys.exit('labels returns %d' % p.returncode)
    labels = {num: name for name, num in json.loads(out).iteritems()}

    with open(CUSTOM_ESTS_FILENAME % est_idx, 'r') as ests_file, \
            open(ANNOT_FILEPATH % est_idx + MXNET_SUFFIX, 'r') as annot_file, \
            open(CUSTOM_ESTS_RES_FILENAME % est_idx, 'w') as ests_res_file:
        est_frags = ests_file.readline().split('\t')
        est = json.loads(est_frags[2])
        for i, line in enumerate(annot_file):
            label_id = int(line.split('\t')[4])
            merge_info = est['merge'][i].split(',')
            est['merge'][i] = '%s,%s,%s' % (merge_info[0], merge_info[1], labels[label_id])
        ests_res_file.write('\t'.join(est_frags[:2]))
        ests_res_file.write('\t')
        ests_res_file.write(json.dumps(est))


def annotate(est_idx):
    with open(ANNOT_FILEPATH % est_idx, 'w') as out:
        print('start annotate %d' % est_idx)
        p = subprocess.Popen(phantom_cmd('annotate', 0, CUSTOM_ESTS_FILENAME % est_idx),
                             stdout=out)
        p.communicate()
        print('finish annotate %d' % est_idx)
    if p.returncode:
        sys.exit('annotate returns %d' % p.returncode)
    os.chdir(ANNOT_DIR)
    p = subprocess.Popen(['nice', './annotate', '-f', ANNOT_FILENAME % est_idx])
    p.communicate()
    if p.returncode:
        sys.exit('matrixnet returns %d' % p.returncode)
    os.chdir('..')

    apply_labels(est_idx)


def main():
    for est_idx in range(ESTS_START, ESTS_END):
        try:
            split(est_idx)
            merge(est_idx)
            annotate(est_idx)
        except Exception:
            print >> sys.stderr, traceback.format_exc()


if __name__ == '__main__':
    main()
