# -*- coding: utf-8 -*-

import argparse
import logging
import os
import random
import shutil
import subprocess
import sys


def parse_args():
    parser = argparse.ArgumentParser(
        description='Learn 3 formulas with STEAM estimations.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-m', dest='mxnet_formulas_dir',
                        default='segmentator_tool/mxnet_top100', help='-')
    parser.add_argument('-t', dest='tmp_factors_dir',
                        default='tmp_learner_factors', help='-')
    parser.add_argument('-d', dest='docs_dir', default='guided_docs',
                        help='-')
    parser.add_argument('-l', dest='docs_list_file', default='docs_list.txt',
                        help='info: must contain docs in the corresponding order for DOCS_ANS_FILE')
    parser.add_argument('-a', dest='docs_ans_file',
                        default='qualifier_data/human_ests.tsv',
                        help='Downloaded from STEAM file with estimations')
    parser.add_argument('--dict', dest='rec_dict_file', required=True,
                        help='recognizer dict file')

    return parser.parse_args()


def get_docs_info(docs_list_file, docs_ans_file):
    logging.info('start get_docs_info')

    docs_info = []
    with open(docs_list_file, 'r') as docs_list, \
            open(docs_ans_file, 'r') as docs_ans:
        cur_doc_id_with_ans = -1
        for line in docs_list:
            line = line.strip()
            fname, url = line.split('\t', 1)
            cur_doc_id = int(fname.split('.', 1)[0])
            while cur_doc_id > cur_doc_id_with_ans:
                doc_ans_line = docs_ans.readline()
                if not doc_ans_line:
                    return docs_info
                cur_doc_id_with_ans = int(doc_ans_line.split('\t', 1)[0]) - 1
            if cur_doc_id == cur_doc_id_with_ans:
                docs_info.append({'name': fname, 'url': url,
                                  'corrupted': False})

    logging.info('finish get_docs_info')
    return docs_info


def bkup_folder(folder_name):
    logging.info('start bkup "%s"' % folder_name)

    # TODO: maybe create folder_name + '.bkup' + timestamp
    folder_bkup_name = folder_name + '.bkup'
    shutil.rmtree(folder_bkup_name, ignore_errors=True)
    shutil.copytree(folder_name, folder_bkup_name)

    logging.info('finish bkup "%s"' % folder_name)


def get_factors_fname(fname, phase, tmp_factors_dir):
    return os.path.join(tmp_factors_dir, '.'.join((fname, phase, 'txt')))


def get_segm_cmd(fname, doc_id, phase, docs_dir, docs_ans_file, rec_dict_file):
    return ['nice', 'segmentator_tool/segmentator_tool',
            '-f', os.path.join(docs_dir, fname),
            '-d', rec_dict_file,
            '-m', 'awareFactors',
            '-a', docs_ans_file,
            '-i', str(doc_id),
            '-p', phase]


def get_mxnet_cmd(phase):
    multi = (phase == 'annotate')
    return ['nice', '../../matrixnet',
            '-m' if multi else '-c',
            '-f', 'learn.tsv',
            '-t', 'test.tsv',
            '-T', '20', '-i', '2000']


def prepare_factors(phase, docs_info, tmp_factors_dir, docs_dir,
                    docs_ans_file, rec_dict_file):
    logging.info('start prepare_factors')

    for doc_id, doc_info in enumerate(docs_info):
        logging.info('doc_id %d' % doc_id)
        if doc_info['corrupted']:
            continue
        fname = doc_info['name']
        with open(get_factors_fname(fname, phase, tmp_factors_dir), 'w') as out:
            p = subprocess.Popen(get_segm_cmd(fname, doc_id, phase, docs_dir,
                                              docs_ans_file, rec_dict_file),
                                 stdout=out)
            p.communicate()
        if p.returncode:
            logging.warn('skip doc: %s (%s) returns %d' %
                         (fname, phase, p.returncode))
            doc_info['corrupted'] = True

    logging.info('finish prepare_factors')


def get_test_sample(sample_len):
    # TODO: maybe split by urls
    test_ratio = 0.25
    return random.sample(range(sample_len), int(test_ratio * sample_len))


def prepare_mxnet_tsvs(phase, docs_info, tmp_factors_dir):
    logging.info('start prepare_mxnet_tsvs')

    recs_count = 0
    mxnet_tsv_file = open(os.path.join(tmp_factors_dir, phase,
                                       'learn_test.tsv'), 'w')
    for doc_info in docs_info:
        if doc_info['corrupted']:
            continue
        fname, url = doc_info['name'], doc_info['url']
        with open(get_factors_fname(fname, phase, tmp_factors_dir),
                  'r') as doc_factors:
            for line in doc_factors:
                guid = line.split('\t', 1)[0]
                if guid == 'UNK':
                    # TODO: maybe use it
                    continue
                mxnet_tsv_file.write(line)
                recs_count += 1
    mxnet_tsv_file.close()

    test_ids = set(get_test_sample(recs_count))
    with open(os.path.join(tmp_factors_dir, phase, 'learn_test.tsv'), 'r') as all_tsv_file, \
            open(os.path.join(tmp_factors_dir, phase, 'learn.tsv'), 'w') as learn_tsv_file, \
            open(os.path.join(tmp_factors_dir, phase, 'test.tsv'), 'w') as test_tsv_file:
        for i, line in enumerate(all_tsv_file):
            if i in test_ids:
                test_tsv_file.write(line)
            else:
                learn_tsv_file.write(line)

    logging.info('finish prepare_mxnet_tsvs')


def calc_mxnet_formula(phase, tmp_factors_dir):
    logging.info('start calc_mxnet_formula')

    cur_dir = os.getcwd()
    os.chdir(os.path.join(tmp_factors_dir, phase))
    p = subprocess.Popen(get_mxnet_cmd(phase))
    p.communicate()
    if p.returncode:
        sys.exit('matrixnet (%s) returns %d' % (phase, p.returncode))
    os.chdir(cur_dir)

    logging.info('finish calc_mxnet_formula')


def replace_segm_formula(phase, mxnet_formulas_dir, tmp_factors_dir):
    logging.info('start replace_segm_formula')

    formula_fname = 'matrixnet.mnmc' if phase == 'annotate' else 'matrixnet.info'
    shutil.copy(os.path.join(tmp_factors_dir, phase, formula_fname),
                os.path.join(mxnet_formulas_dir, phase))

    logging.info('finish replace_segm_formula')


def recompile_segm():
    logging.info('start recompile_segm')

    p = subprocess.Popen(['nice', 'ya', 'build', '--rebuild',
                          '-j', '24', 'segmentator_tool'])
    p.communicate()
    if p.returncode:
        sys.exit('ya build returns %d' % p.returncode)

    logging.info('finish recompile_segm')


def process(phase, docs_info, args):
    prepare_factors(phase, docs_info, args.tmp_factors_dir, args.docs_dir,
                    args.docs_ans_file, args.rec_dict_file)
    prepare_mxnet_tsvs(phase, docs_info, args.tmp_factors_dir)
    calc_mxnet_formula(phase, args.tmp_factors_dir)
    replace_segm_formula(phase, args.mxnet_formulas_dir, args.tmp_factors_dir)


def main():
    args = parse_args()
    logging.basicConfig(format='%(levelname)s: %(message)s',
                        stream=sys.stderr, level=logging.DEBUG)

    bkup_folder(args.mxnet_formulas_dir)
    docs_info = get_docs_info(args.docs_list_file, args.docs_ans_file)
    for phase in ('split', 'merge', 'annotate'):
        logging.info('START PHASE "%s"' % phase)
        process(phase, docs_info, args)
    recompile_segm()


if __name__ == '__main__':
    main()
