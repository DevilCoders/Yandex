#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import yatest.common

import json

CSNIP_PATH = yatest.common.binary_path('tools/snipmake/csnip/csnip')

CONTEXT_DATA_PATH = yatest.common.work_path('25_contexts_for_4_queries.tsv')

SNIPPET_DUMP_OUTPUT_PATH_NO_META_DESC = yatest.common.output_path('csnip_output_no_meta_desc.tsv')
CANONICAL_OUTPUT_PATH_NO_META_DESC = yatest.common.output_path('additional_snippets_no_meta_desc.tsv')

# *** The following queries were used to produce the contexts from sbr://1969786305: ***
# *** Contexts were produced for www (snip_width=536), www-touch (324) and www-tablet (584) reports ***
# как спят змеи
# что такое мэйк
# молитва о здоровье
# цитаты стетхема


def try_extract_additional_snippet(csnip_data):
    clicklike_json = csnip_data.get('clicklike_json', None)

    if clicklike_json is not None:
        fact_snip = json.loads(clicklike_json).get('fact_snip', None)
        if fact_snip is None:
            return '-'
        fact_snip_candidates = fact_snip.get('fact_snip_candidates')
        if fact_snip_candidates is not None:
            result = ''
            for candidate in fact_snip_candidates:
                result += "[" + candidate['text'] + "], "
            return result
        else:
            return '-'
    else:
        return '-'


def test_additional_snippets_no_meta():
    with open(SNIPPET_DUMP_OUTPUT_PATH_NO_META_DESC, 'w') as fout:
        yatest.common.execute(
            [
                CSNIP_PATH,
                '-r', 'json',
                '-E', 'fact_snip_candidates,fact_snip_top_candidate_count=10,fact_snip_ignore_meta_descr',
                '-i', CONTEXT_DATA_PATH,
            ],
            stdout=fout,
            check_exit_code=True
        )

    with open(CANONICAL_OUTPUT_PATH_NO_META_DESC, 'w') as fout:
        print('query\turl\fact_snip_candidates', file=fout)
        for line in open(SNIPPET_DUMP_OUTPUT_PATH_NO_META_DESC):
            csnip_data = json.loads(line.strip())
            maybe_additional_snippet = try_extract_additional_snippet(csnip_data)
            print('{}\t{}\t{}'.format(
                csnip_data['userreq'].encode('utf8'),
                csnip_data['url'].encode('utf8'),
                maybe_additional_snippet.encode('utf8')
            ), file=fout)
    return yatest.common.canonical_file(CANONICAL_OUTPUT_PATH_NO_META_DESC)
