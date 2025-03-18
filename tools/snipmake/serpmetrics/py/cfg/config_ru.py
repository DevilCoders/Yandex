#!/usr/bin/python
# -*- coding: utf-8 -*-

data_folder     = "./data_ru/"
serp_type       = {"country":"RU"}
plot_name       = "Автометрики - RU"
plot_type       = "ru"
streams = [
    {"name":"Я", "file":"yx_lemma", "sysId":138},
    {"name":"G", "file":"gg_clean", "sysId":139},
    {"name":"Я.Приемка", "file":"ya_fixbin_ru", "sysId":400},
    {"name":"G.Приемка", "file":"gg_fixbin_ru", "sysId":401},
    {"name":"Y.WEB_RU_MIXED_VALIDATE_DEDUPLICATED", "file":"ya_mixed_validate_deduplicated", "sysId":1457},
    {"name":"G.WEB_RU_MIXED_VALIDATE_DEDUPLICATED", "file":"gg_mixed_validate_deduplicated", "sysId":1459},
    {"name":"External", "file":"ya_external_ru"}
]

razladki_streams = dict([
    ("ya_mixed_validate_deduplicated", "ru|desktop|mixed_dedup"),
    ("gg_mixed_validate_deduplicated", "ru|desktop|gg_mixed_dedup")
])
