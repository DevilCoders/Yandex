#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import yatest.common

import json

CSNIP_PATH = yatest.common.binary_path('tools/snipmake/csnip/csnip')
DUMP_FACTSNIP_FACTOR_PATH = yatest.common.binary_path('tools/snipmake/dump_factsnip_factor/dump_factsnip_factor')

CONTEXT_DATA_PATH = yatest.common.work_path('200_contexts_for_20_queries.tsv')
DSSM_PATH = yatest.common.work_path('RuFactSnippet.dssm')

SNIPPET_DUMP_OUTPUT_PATH = yatest.common.output_path('csnip_output.tsv')
BASE64_FACTORS_OUTPUT_PATH = yatest.common.output_path('base64_factors.tsv')
FLOAT_DSSM_FACTORS_OUTPUT_PATH = yatest.common.output_path('float_dssm_factors.tsv')

CANONICAL_DSSM_FACTORS_PATH = yatest.common.output_path('canonical_dssm_factors_approximated.tsv')

# *** The following queries were used to produce the contexts from sbr://577073228: ***
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


def try_extract_base64_factsnip_factors(csnip_out):
    clicklike_json = json.loads(csnip_out).get('clicklike_json', None)

    if clicklike_json is not None:
        fact_snip = json.loads(clicklike_json).get('fact_snip', None)
        if fact_snip is not None:
            return fact_snip['factors']
        else:
            return None
    else:
        return None


def test_fact_snippet_dssm_factor():
    with open(SNIPPET_DUMP_OUTPUT_PATH, 'w') as fout:
        yatest.common.execute(
            [
                CSNIP_PATH,
                '-r', 'json',
                '-E', 'finaldump,get_query,use_factsnip',
                '-i', CONTEXT_DATA_PATH,
                '--dssm-applier', DSSM_PATH,
            ],
            stdout=fout,
            check_exit_code=True
        )

    with open(BASE64_FACTORS_OUTPUT_PATH, 'w') as fout:
        for line in open(SNIPPET_DUMP_OUTPUT_PATH):
            maybe_base64_factors = try_extract_base64_factsnip_factors(line.strip())
            if maybe_base64_factors is not None:
                print(maybe_base64_factors, file=fout)

    yatest.common.execute(
        [
            DUMP_FACTSNIP_FACTOR_PATH,
            '-i', BASE64_FACTORS_OUTPUT_PATH,
            '-o', FLOAT_DSSM_FACTORS_OUTPUT_PATH
        ]
    )

    with open(CANONICAL_DSSM_FACTORS_PATH, 'w') as fout:
        print('\n'.join(['{0:.3f}'.format(float(line.strip())) for line in open(FLOAT_DSSM_FACTORS_OUTPUT_PATH)]), file=fout)

    return yatest.common.canonical_file(CANONICAL_DSSM_FACTORS_PATH, local=True)
