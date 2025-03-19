#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import yatest.common

import json

CSNIP_PATH = yatest.common.binary_path('tools/snipmake/csnip/csnip')

CONTEXT_DATA_PATH = yatest.common.work_path('600_contexts_for_20_queries_desktop_touch_pad.tsv')

SNIPPET_DUMP_OUTPUT_PATH = yatest.common.output_path('csnip_output.tsv')
CANONICAL_OUTPUT_PATH = yatest.common.output_path('extended_snippets.tsv')

# *** The following queries were used to produce the contexts from sbr://577073228: ***
# *** Contexts were produced for www (snip_width=536), www-touch (324) and www-tablet (584) reports ***
# лизинг это
# 160 ук рф присвоение или растрата
# ts качество что это
# ტრანზისტორი происхождение слова
# өткізгіштерді параллель жалғау дегеніміз не
# матэ это
# 0 00348 btc
# 1 10 легиона сканворд 7 букв
# ン происхождение слова
# 12 июня праздник
# актуальность
# адрес деда мороза
# но в тоже время слитно или раздельно
# ниагарачто за ткань
# производственный календарь на 2018 год
# jvtuf-3
# сколько станций в московской подземке на 2018 год
# высотная поясность где находится
# какова масса французского бульдога
# а и б сидели на трубе а упало б пропало кто остался на трубе


def try_extract_extended_snippet(csnip_data):
    clicklike_json = csnip_data.get('clicklike_json', None)

    if clicklike_json is not None:
        extended_snippet = json.loads(clicklike_json).get('extended_snippet', None)
        if extended_snippet is not None:
            return '... '.join(extended_snippet['features']['passages'])
        else:
            return '-'
    else:
        return '-'


def test_extended_snippet():
    with open(SNIPPET_DUMP_OUTPUT_PATH, 'w') as fout:
        yatest.common.execute(
            [
                CSNIP_PATH,
                '-r', 'json',
                '-i', CONTEXT_DATA_PATH,
            ],
            stdout=fout,
            check_exit_code=True
        )

    with open(CANONICAL_OUTPUT_PATH, 'w') as fout:
        print('query\turl\tsnip_width\textended_snippet', file=fout)
        for line in open(SNIPPET_DUMP_OUTPUT_PATH):
            csnip_data = json.loads(line.strip())
            maybe_extended_snippet = try_extract_extended_snippet(csnip_data)
            print('{}\t{}\t{}\t{}'.format(
                csnip_data['userreq'].encode('utf8'),
                csnip_data['url'].encode('utf8'),
                csnip_data['snip_width'].encode('utf8'),
                maybe_extended_snippet.encode('utf8')
            ), file=fout)

    return yatest.common.canonical_file(CANONICAL_OUTPUT_PATH)
