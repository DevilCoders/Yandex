# -*- coding: utf-8 -*-

import argparse
import codecs
import difflib
from operator import itemgetter
import os
import re
import subprocess
import sys


DELIMITER = u'<-------------------------------------------------->'

TEST_TO_PROD_SEGS = {
    'AUX': ('Segment-aux',),
    'AAD': ('Segment-aux', 'Segment-links'),
    'ACP': ('Segment-aux', 'Segment-footer'),
    'AIN': ('Segment-aux',),
    'ASL': ('Segment-links',),
    'DMD': ('Segment-aux', 'Segment-footer'),
    'DHC': ('Segment-header',),
    'DHA': ('Segment-aux', 'Segment-header'),
    'DCT': ('Segment-content',),
    'DCM': ('Segment-content',),
    'LMN': ('Segment-menu',),
    'LCN': ('Segment-referat',),  # ?
    'LIN': ('Segment-links',),
}

SEG_TO_GROUP = {
    'Segment-aux': 1,
    'Segment-links': 1,
    'Segment-footer': 1,
    'Segment-header': 2,
    'Segment-content': 2,
    'Segment-menu': 3,
    'Segment-referat': 3,
    'AUX': 1,
    'AAD': 1,
    'ACP': 1,
    'AIN': 1,
    'ASL': 1,
    'DMD': 2,
    'DHC': 2,
    'DHA': 2,
    'DCT': 2,
    'DCM': 2,
    'LMN': 3,
    'LCN': 3,
    'LIN': 3,
}


def parse_args():
    parser = argparse.ArgumentParser(
        description='Calculate quality of test and production segmentators versions with STEAM estimations.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-c', dest='human_sents_file',
                        default='qualifier_data/human_sents.txt', help='-')
    parser.add_argument('-p', dest='prod_sents_file',
                        default='qualifier_data/prod_sents.txt', help='-')
    parser.add_argument('-e', dest='human_ests_file',
                        default='qualifier_data/human_ests.tsv', help='-')
    parser.add_argument('-t', dest='tmp_segm_dir',
                        default='tmp_qualifier_segm', help='-')
    parser.add_argument('-d', dest='docs_dir',
                        default='qualifier_data/docs', help='-')
    parser.add_argument('--dict', dest='rec_dict_file', required=True,
                        help='recognizer dict file')

    return parser.parse_args()


def get_tmp_segm_name(fname, tmp_segm_dir):
    return os.path.join(tmp_segm_dir, fname)


def get_tmp_sent_name(segm_fname):
    return segm_fname + '.sent'


def get_segm_cmd(fname, docs_dir, rec_dict_file):
    return ['nice', 'segmentator_tool/segmentator_tool',
            '-f', os.path.join(docs_dir, fname),
            '-d', rec_dict_file,
            '-m', 'apply']


def split_segments(segm_fname):
    def write_sents(out, cur_label, cur_sents):
        # skip [start_pos, end_pos]
        cur_sents = cur_sents[1:]
        if not cur_sents:
            return
        # skip "'"s
        cur_sents[0] = cur_sents[0][1:].lstrip()
        cur_sents[-1] = cur_sents[-1][:-1].rstrip()
        for sent in cur_sents:
            if sent:
                out.write(u'%s\t%s\n' % (cur_label, sent))

    label_re = re.compile(r'Label: [A-Z]+$', re.U)
    with codecs.open(segm_fname, 'r', 'utf-8') as inp, \
            codecs.open(get_tmp_sent_name(segm_fname), 'w', 'utf-8') as out:
        cur_label = None
        cur_sents = []
        for line in inp:
            line = line.strip()
            if label_re.match(line):
                write_sents(out, cur_label, cur_sents)
                cur_label = line.split(u' ')[1]
                cur_sents = []
            else:
                if cur_label is None:
                    continue
                cur_sents.append(line)
        write_sents(out, cur_label, cur_sents)


def parse_sent_line(line):
    seg, sent = line.split(u'\t', 1)
    sent = sent.strip()
    return (seg, sent)


def get_next_corr_sents(corr_segm_file):
    corr_sents = []
    for line in corr_segm_file:
        line = line.strip()
        if line == DELIMITER:
            break
        corr_sents.append(parse_sent_line(line))
    return corr_sents


def get_prod_sents(doc_id, prod_segm_file):
    if not hasattr(get_prod_sents, 'doc_id'):
        get_prod_sents.doc_id = 0
    while get_prod_sents.doc_id < doc_id:
        get_prod_sents.doc_id += 1
        for line in prod_segm_file:
            if line.strip() == DELIMITER:
                break

    prod_sents = []
    for line in prod_segm_file:
        line = line.strip()
        if not line:
            break
        prod_sents.append(parse_sent_line(line))
    return prod_sents


def get_test_sents(segm_sents):
    test_sents = []
    for line in segm_sents:
        line = line.strip()
        test_sents.append(parse_sent_line(line))
    return test_sents


def get_matched_sents(sents1, sents2, sents3):
    matches12 = difflib.SequenceMatcher(None,
                                        sents1, sents2).get_matching_blocks()
    matches13 = difflib.SequenceMatcher(None,
                                        sents1, sents3).get_matching_blocks()
    i = j = 0
    matches = []
    while i < len(matches12) and j < len(matches13):
        match1_2 = (matches12[i][0], matches12[i][0] + matches12[i][2])
        match1_3 = (matches13[j][0], matches13[j][0] + matches13[j][2])
        common_match = (max(match1_2[0], match1_3[0]),
                        min(match1_2[1], match1_3[1]))
        common_len = common_match[1] - common_match[0]
        if common_len > 0:
            matches.append((common_match[0],
                            matches12[i][1] + common_match[0] - matches12[i][0],
                            matches13[j][1] + common_match[0] - matches13[j][0],
                            common_len))
        if match1_2[1] < match1_3[1]:
            i += 1
        else:
            j += 1
    return matches


def get_quality(doc_id, segm_fname, corr_segm_file, prod_segm_file):
    corr_sents = get_next_corr_sents(corr_segm_file)
    prod_sents = get_prod_sents(doc_id, prod_segm_file)
    with codecs.open(get_tmp_sent_name(segm_fname), 'r', 'utf-8') as segm_sents:
        test_sents = get_test_sents(segm_sents)

    matches = get_matched_sents(map(itemgetter(1), corr_sents),
                                          map(itemgetter(1), prod_sents),
                                          map(itemgetter(1), test_sents))
    print('correct=%d, prod=%d, test=%d, matches=%d' % (
        len(corr_sents), len(prod_sents), len(test_sents),
        sum(i[3] for i in matches)))

    test_score = prod_score = 0
    test_group_score = prod_group_score = 0
    sent_count = 0
    for corr_start, prod_start, test_start, count in matches:
        for i in range(count):
            corr_new_seg = corr_sents[corr_start + i][0]
            test_new_seg = test_sents[test_start + i][0]
            corr_seg = TEST_TO_PROD_SEGS[corr_new_seg]
            test_seg = TEST_TO_PROD_SEGS[test_new_seg]
            prod_seg = prod_sents[prod_start + i][0]
            corr_seg_set = set(corr_seg)
            if corr_seg_set.intersection(test_seg):
                test_score += 1
            if corr_seg_set.intersection([prod_seg]):
                prod_score += 1
            if SEG_TO_GROUP[corr_new_seg] == SEG_TO_GROUP[test_new_seg]:
                test_group_score += 1
            if SEG_TO_GROUP[corr_new_seg] == SEG_TO_GROUP[prod_seg]:
                prod_group_score += 1
        sent_count += count

    return (test_score, prod_score, test_group_score, prod_group_score,
            sent_count)



def process(doc_id, file_doc_id, corr_segm_file, prod_segm_file, args):
    fname = '%d.txt' % file_doc_id
    segm_fname = get_tmp_segm_name(fname, args.tmp_segm_dir)
    with codecs.open(segm_fname, 'w', 'utf-8') as out:
        p = subprocess.Popen(get_segm_cmd(fname, args.docs_dir,
                                          args.rec_dict_file), stdout=out)
        p.communicate()
    if p.returncode:
        sys.exit('segmentator_tool with "%s" returns %d' % (fname,
                                                            p.returncode))

    split_segments(segm_fname)

    return get_quality(doc_id, segm_fname, corr_segm_file, prod_segm_file)


def main():
    args = parse_args()
    total_test_score = total_prod_score = 0
    total_test_group_score = total_prod_group_score = 0
    total_sent_count = 0
    corr_segm_file = codecs.open(args.human_sents_file, 'r', 'utf-8')
    prod_segm_file = codecs.open(args.prod_sents_file, 'r', 'utf-8')
    corr_segm_ests_file = codecs.open(args.human_ests_file, 'r', 'utf-8')
    for line in corr_segm_ests_file:
        doc_id = int(line.split('\t', 1)[0])
        file_doc_id = doc_id - 1
        print('doc_id=%d' % file_doc_id)
        test_score, prod_score, test_group_score, prod_group_score, sent_count = \
            process(doc_id, file_doc_id, corr_segm_file, prod_segm_file, args)
        print('abs: test=%d, prod=%d' % (test_score, prod_score))
        print('group: test=%d, prod=%d\n' % (test_group_score, prod_group_score))
        total_test_score += test_score
        total_prod_score += prod_score
        total_test_group_score += test_group_score
        total_prod_group_score += prod_group_score
        total_sent_count += sent_count
    corr_segm_file.close()
    prod_segm_file.close()
    corr_segm_ests_file.close()
    print('total sents: %d' % total_sent_count)
    print('total abs: test=%d, prod=%d' % (total_test_score, total_prod_score))
    print('total group: test=%d, prod=%d\n' % (total_test_group_score,
                                               total_prod_group_score))


if __name__ == '__main__':
    main()
