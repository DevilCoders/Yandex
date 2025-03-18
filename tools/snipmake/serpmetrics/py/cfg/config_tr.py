#!/usr/bin/python
# -*- coding: utf-8 -*-

data_folder     = "./data_tr/"
serp_type       = {"country":"TR"}
plot_name       = "Автометрики - TR"
plot_type       = "tr"
streams = [
    {"name":"Y.TR.Validate", "file":"yx_lemma_tr", "sysId":1611},
    {"name":"G.TR.Validate", "file":"gg_clean_tr", "sysId":1613},
    {"name":"Я.Tr.Приемка", "file":"ya_fixbin_tr", "sysId":3},
    {"name":"G.TR_MIXED_DEDUPLICATED", "file":"gg_mixed_deduplicated", "sysId":1305},
    {"name":"Y.TR_MIXED_DEDUPLICATED", "file":"ya_mixed_deduplicated", "sysId":1303},
]

razladki_streams = dict([
    ("ya_mixed_deduplicated", "tr|desktop|mixed_dedup"),
    ("gg_mixed_deduplicated", "tr|desktop|gg_mixed_dedup")
])
