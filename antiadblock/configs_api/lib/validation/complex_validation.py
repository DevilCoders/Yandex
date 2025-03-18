# coding=utf-8
import flask
import re2
from itertools import product
from datetime import datetime, timedelta

import jwt
from jwt.exceptions import DecodeError, InvalidTokenError, ExpiredSignatureError, InvalidSignatureError
from voluptuous import Invalid, MultipleInvalid, Required, Length, Schema, ALLOW_EXTRA, Optional, All, Any, Range

from antiadblock.configs_api.lib.utils import URLBuilder
from antiadblock.configs_api.lib.const import EXPERIMENT_DATETIME_FORMAT
from antiadblock.configs_api.lib.validation.template import TOKENS_VALIDATOR, TEST_DATA_VALIDATOR, compilable_re, validate_ts, UserDevice, Experiments
from antiadblock.configs_api.lib.validation.validation_utils import exception_to_invalid, is_schema_ok, return_validation_errors

# TODO: remove HARDCODED_TOKENS https://st.yandex-team.ru/ANTIADB-2124
HARDCODED_TOKENS = {
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTMwOTAwOTUsInN1YiI6InlhbmRleF92aWRlbyIsImV4cCI6MTU0NDYzNjg5NX0.m_S2RtMcTOHEwjl_6BhhXvzP7FDPTU3r_wCQ8H_sGeU": "yandex_video",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDg3NTYyNjgsInN1YiI6InJlZ21hcmtldHMiLCJleHAiOjE1MjQ0OTE4Njh9.3XcNdo9zZMkjt6VbrATtZDACX51IST_Zr_TA1-vHz9k": "ria_inosmi",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTYwMjEwMjksInN1YiI6Imhvcm9zY29wZXMiLCJleHAiOjE1NDc1Njc4Mjl9.D6gbsZykr9W6Ylx6k-Dm00ON3GV1IJxB-I8kRmw2QWg": "rambler_horoscopes",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNwb3J0X3JiYyIsImV4cCI6MTUyMDM0NTc4OX0.zGByXbwuQJB7oFbbnK1VW--mHGXFPimX3BRrsOm5uf4": "sport_rbc",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDU0Njg4ODIsInN1YiI6ImRyaXZlMiIsImV4cCI6MTUyMTIwNDQ4Mn0.o8u-RuYC9WYdYDzpIcfnBiohHUX2ht4T_pqyZMnH-X0": "drive2",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDU3NDQ5MjMsInN1YiI6InlhbmRleF9tb3JkYSIsImV4cCI6MTUyMTQ4MDUyM30.cE1QCUI0rUSOvOy876ilmYQ2HOxs3jHlFF6q-xs327c": "yandex_morda",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InRpZW5zbWVkIiwiZXhwIjoxNTIwMzQ1Nzg5fQ.ZinfjU8Edo1Z7IY-GTp_PVzaPPjmsry1jJHzSM6wsrY": "infoniac",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDY0MzY3NTksInN1YiI6InBpa2FidSIsImV4cCI6MTUyMjE3MjM1OX0.Gq5MAUrJTymxLDbuR-JxDjV2e3AsOvktNIQbtXpaCYk": "pikabu",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTAzMDg0MjQsInN1YiI6ImVjaG9tc2siLCJleHAiOjE1MjYwNDQwMjR9.SLI_fgk6OtX86vrDwQoeIaMoaYtzOPSTQcAAlOArPFo": "echomsk",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImJsYW1wZXIiLCJleHAiOjE1MjAzNDU3ODl9.xJLha1Xg3BAPlW4ZtdbikGb1Pw6AA1rBJt02hjrZLys": "blamper",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ3ODk1MTgsInN1YiI6InByb3BhcnRuZXIiLCJleHAiOjE1MjA1MjUxMTh9.zQLW9_pbJq3msLC82eevbiRHR_XNwhmGJVWHpMPTFns": "propartner",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDY5MzI1ODMsInN1YiI6ImZpbmFtcnUiLCJleHAiOjE1MjI2NjgxODN9.GtpU9oiqfkWCVmSiGMMh0pN7vvkxXlDXO1rBSaq3TCo": "rusprofile",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InlhbmRleF9tYWlsIiwiZXhwIjoxNTIwMzQ1Nzg5fQ.48uTbJKS-p_poqPh129qXPMbaUOvTzc677VjF4jQ43E": "yandex_mail",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTE1MTU1NDAsInN1YiI6ImV4YW1wbGVfcGFnZSIsImV4cCI6MTU0MzA2MjM0MH0.6U8n-CJKHDmY02_l4Jsn4HWD1lm5rdUGfdlWJM4INGk": "example_page",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImRuaSIsImV4cCI6MTUyMDM0NTc4OX0.zVkuKN41iQs_1KZLaL5_lRL5ZQCYQm9Ky_PufDfxhEE": "dni",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ3ODg0NTcsInN1YiI6ImJhYnlydSIsImV4cCI6MTUyMDUyNDA1N30.0tFSx1zILjNq5cIWk4-JeKXDs_1w6_P-PqHI4xH_-Nw": "babyru",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InBvZ29kYV90ZXN0IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.kjQIcd7cORZ2frUIjI3U8EXVthjd8NFTUW9X8OWgIao": "yandex_pogoda_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ3MTU2NzEsInN1YiI6ImRldGlfb25saW5lIiwiZXhwIjoxNTIwNDUxMjcxfQ.YpN4vHS9vQs4y_5H_BJ7bfUOSlN-CB3ebLb6ivH0ipw": "deti_online",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImF1dG9ydSIsImV4cCI6MTUyMDM0NTc4OX0.QJF0jxrxbHK-xTYwktSSue74Wx-Pqiw-iTt2nqW0Q1g": "autoru",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTcyMzc0NzEsInN1YiI6InRlc3QiLCJleHAiOjE1NDg3ODQyNzF9.qXJFWfcrawwMvmF_ZEt-nZ0K874Cl-IH5P9ntRCPZfA": "test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDk0NDY5NTgsInN1YiI6InJlZ251bSIsImV4cCI6MTUyNTE4MjU1OH0.t5nkSx7Svo9WfV3-GtSuEzu58HH72LVQ73qDDJ6clLQ": "regnum",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InRlc3QiLCJleHAiOjE1MjAzNDU3ODl9.G_O4oXwTmV4Cq3LUr7dHaXqdimzW2_p1PZRhOBMEIDA": "test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InJlZ21hcmtldHMiLCJleHAiOjE1MjAzNDU3ODl9.8uMyh8YFAK3ilBLkNFof76qKVJtq7cZVvGNolOqSkYw": "regmarkets",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDUxMzkzMTksInN1YiI6Im1rIiwiZXhwIjoxNTIwODc0OTE5fQ.nuAhLGxqRJaDS5vWInSWDVSfJt1yt1YAgIeHCrBtS0E": "mk",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImZpc2hraSIsImV4cCI6MTUyMDM0NTc4OX0.pE1nhOC3LPDcifezYBZkPmbdb_wXNuDsiU_5evW5RaM": "fishki",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDk2MDkxODQsInN1YiI6InJlZ21hcmtldHMiLCJleHAiOjE1MjUzNDQ3ODR9.syYXJctttT8PyxrjEOD-kxLCm6du1Icm7EEIkMDtUcY": "yandex_news",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImF1dG9ydV90ZXN0IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.bZ1Oq4QjKmfefDZv1tKMxio_GS9aFa-JOR9TCYg00mc": "autoru_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTMwODk5MjAsInN1YiI6InlhbmRleF9pbWFnZXMiLCJleHAiOjE1NDQ2MzY3MjB9.lvwan5Metpf_vSjLs2hNeF6ekKCc095jy9cPY_msiIE": "yandex_images",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTE4Njk2NDgsInN1YiI6InlhbmRleF9hZmlzaGEiLCJleHAiOjE1NDM0MTY0NDh9.S6mXAooufGoYUHg9m6GZiNZVxR0XRB1lmKUTzlQUoEg": "yandex_afisha",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDY0MjYwNjQsInN1YiI6ImxpdmVpbnRlcm5ldCIsImV4cCI6MTUyMjE2MTY2NH0.BrSw6NAj_ftcH9AcpuuhtOBbNA06dmBWCE4vBNA5u18": "liveinternet",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDUyMzA1NTcsInN1YiI6Imdvcm9kX3JhYm90IiwiZXhwIjoxNTIwOTY2MTU3fQ.i1y-emEnhUvLnBiU9hNDYVsOG6RYE0B6QsSzVgJ-f2o": "gorod_rabot",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNkYW1naWEiLCJleHAiOjE1MjAzNDU3ODl9.J3gKeVQOzfpOuRJEPhmsSOxg2HGyOYQ9tFHMwsMqJfs": "sdamgia",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ4ODk2MzEsInN1YiI6ImZpbmFtcnUiLCJleHAiOjE1MjA2MjUyMzF9.bcvtp-mnjjFf028gAniPjnY2KiAS24toLqti4Q-ExVo": "finamru",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InlhbmRleF9zZWFyY2giLCJleHAiOjE1MjAzNDU3ODl9.RqOg_czyY0FfZlGkFpRdL26V6eLzxSFHgfdz06BXf0k": "yandex_search",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ3MTk5MjYsInN1YiI6ImZhY3Ryb29tIiwiZXhwIjoxNTIwNDU1NTI2fQ.zRlzMaSMv3al0lH5Co01jdAjLkqPn1Qo8TdUBlmGH8w": "factroom",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDUxMzkzMTksInN1YiI6Im1rX290aGVyIiwiZXhwIjoxNTIwODc0OTE5fQ.Pf4ko50javAA-vFu2eKGa7sFmg8aFwVqWijpeCqJE2E": "mk_other",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDUzMTc1OTMsInN1YiI6Imdpc21ldGVvIiwiZXhwIjoxNTIxMDUzMTkzfQ.O-pERza2Io3Hrp0f4TH4PFCQFL11zQABl5RRYSnun_0": "gismeteo",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6Imtha3Byb3N0b3J1X3Rlc3QiLCJleHAiOjE1MjAzNDU3ODl9.JkbYJTcxOrwZ-2rxzSTUFc1T9hrSXRDKV8lkQs4bWp4":
        "kakprostoru_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTA5MTgxMTYsInN1YiI6ImxpdmVqb3VybmFsIiwiZXhwIjoxNTQyNDY0OTE2fQ.HOkN4xaOgSAdMjA0Sf3fGJJDxvd2TMrtH7npFC5TCSU": "livejournal",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6Im90em92aWsiLCJleHAiOjE1MjAzNDU3ODl9.MsHpik4Mt-cRZ5v7lfdiIrzfaBmzTJu9CFC5h7od-48": "otzovik",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDUxMzIxOTgsInN1YiI6ImRyaXZlMiIsImV4cCI6MTUyMDg2Nzc5OH0.ey55i3WY_sroD_ypHVQFtVpMjJCswO3_-wA9Ad3nhGk": "drive2_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InJhemxvemhpIiwiZXhwIjoxNTIwMzQ1Nzg5fQ.HNkajMZtqpslzElg2x70XoyFSqv3tKygeJxZC1Ppc_w": "razlozhi",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6Imtha3Byb3N0b3J1IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.YXBFEar1aqaYpj4wJ3eToJpM0m3TNbPtqyyYyeZZtrU": "kakprostoru",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNvdmV0cyIsImV4cCI6MTUyMDM0NTc4OX0.Sysik2gtm_U1nwVd8182EU82BVcANbugI7NOsx38NMU": "sovets",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InZ6Z2x5YWQiLCJleHAiOjE1MjAzNDU3ODl9.6n4rs7x_BBugJMtBe9MMv0T6Uh9TPFwDbJ7EGINwnWo": "vzglyad",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InN5bHJ1IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.RXrF995TfCLs0zS6U9pyIqaXEUNuO3eYV6YGBqbaxDU": "sylru",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNtaTIiLCJleHAiOjE1MjAzNDU3ODl9.dFCShfUtmpEj49N2l6TjxRDulTBXvJWLyXDHvRblDBc": "smi2",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTQwNjk1NzYsInN1YiI6InByb2ZpbmFuY2UiLCJleHAiOjE1NDU2MTYzNzZ9.r3xswR6N0B02TzHzIcA8EWp97FnmThdPteUliDREQgw": "profinance",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InBvZ29kYSIsImV4cCI6MTUyMDM0NTc4OX0.h-TST9YyWZijfKzH7V4sXlBBK3vGMX6LT3dshgqdKok": "yandex_pogoda",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImluZm94IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.NDwHIRhFyyRY1Tuw1trvktOaIEAJ4XGUlS2eTlkyNGg": "infox",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6Imlub3ByZXNzYSIsImV4cCI6MTUyMDM0NTc4OX0.8Isq_XQJN1-b_r2G7xjZ3CuLZtke95HWYOMleY5Ha4Q": "inopressa",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ3MTk5MjYsInN1YiI6ImUxIiwiZXhwIjoxNTIwNDU1NTI2fQ.aRDW0FDvul1m0B71kaZi3kcYMhMjgXSI4wC0hn4FXX4": "e1",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNwcmF2a3VzIiwiZXhwIjoxNTIwMzQ1Nzg5fQ.4F8o9jUN6j9X8hBVzN4Y-ittG4P75hOlajRGTjZ6VIc": "spravkus",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTU3NTkyODYsInN1YiI6InlhbmRleF9zaWJfdGVzdCIsImV4cCI6MTU0NzMwNjA4Nn0.Pc1yhp-A8gd22r_mOlsXWPsUGw-dhlFwkLz6k_dgCZ0":
        "yandex_sib_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6Im90em92aWtfdGVzdCIsImV4cCI6MTUyMDM0NTc4OX0.GI0hlNak2SP0WWGJdSbZ00LLrXcZcBNsv83glibAuWM": "otzovik_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImZicnUiLCJleHAiOjE1MjAzNDU3ODl9.wAsi6wBBnjJe0wUneY1WaqbAgVUM5bioWauYqKYxCzo": "fbru",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNtaTI0IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.YKgbCf1t4uXlnE6DARjlARLyKuMbUOvhgwF8RlCc2Fs": "smi24",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InNwb3J0X3JiY190ZXN0IiwiZXhwIjoxNTIwMzQ1Nzg5fQ.HHhPOSilJlRWMjHkk8JSXr5qa3B5luWu7hlXpl6K38M": "sport_rbc_test",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDk1NDEzNzksInN1YiI6Im1pcnRlc2VuIiwiZXhwIjoxNTI1Mjc2OTc5fQ.eoiFNgmv-q67Qi7SBg6WEj7joFR1e_7YbX3W70Ibn6c": "mirtesen",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6ImlyZWNvbW1lbmQiLCJleHAiOjE1MjAzNDU3ODl9.OH9jgZPnp4bJs6Uc1qxUmMvcQtpbtxnZ7KQOXVTDqy4": "irecommend",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InR2X3Byb2dyYW1tYSIsImV4cCI6MTUyMDM0NTc4OX0.73ZL3_lDGZ09VyvB4x-KJnNttbH4vM9rCciCrMu-VUI": "yandex_tv",
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTc0MDg3MjYsInN1YiI6InlhbmRleF9yZWFsdHkiLCJleHAiOjE1NDg5NTU1MjZ9.QIatmkgfRGAIovOROxP7ynRUYCF-D5_WW5hjBhFpgdg": "yandex_realty"}


@exception_to_invalid
def run_static_tests(config_data):
    """
    Runs checks from "TEST_DATA" method. If there is no "paths" in TEST_DATA then test valid / invalid domains against PROXY_URL_RE + CRYPT_URL_RE.
    If 'paths' is in TEST_DATA, than took combination of valid / invalid domains and test this combinations against PROXY_URL_RE + CRYPT_URL_RE
    :param config_data: data field from config
    :return: config if everything is ok
    :raises MultipleInvalid if there were errors
    """

    def is_compilable(regexp):
        return is_schema_ok(Schema(compilable_re), regexp)

    def is_match(regexps, domain):
        for re in regexps:
            # This should fall on static check
            if not is_compilable(re):
                continue
            reg_ex = re2.compile(re)
            if reg_ex.match(str(URLBuilder(domain)[""])):
                return True
        return False

    @return_validation_errors(prefix_path=["TEST_DATA"])
    def check_domains_present(config_data):
        Schema({Required("invalid_domains", msg=u"Поле {} является обязательным".format('invalid_domains')): All([basestring], Length(min=1, msg=u"Поле должно содержать хотя бы одну запись")),
                Required("valid_domains", msg=u"Поле {} является обязательным".format('valid_domains')): All([basestring], Length(min=1, msg=u"Поле должно содержать хотя бы одну запись")),
                }, extra=ALLOW_EXTRA)(config_data["TEST_DATA"])

    @return_validation_errors(prefix_path=["TEST_DATA"])
    def check_paths_present(config_data):
        Schema({Optional("paths"): All(Length(min=1, msg=u"Поле должно содержать хотя бы один валидный или невалидный домен"),
                                       {Optional("valid"): All([basestring], Length(min=1, msg=u"Поле должно содержать хотя бы одну запись")),
                                        Optional("invalid"): All([basestring], Length(min=1, msg=u"Поле должно содержать хотя бы одну запись"))},
                                       )}, extra=ALLOW_EXTRA)(config_data["TEST_DATA"])

    def get_errors(config_data):
        errors = []
        proxy_urls = config_data.get("PROXY_URL_RE", []) + config_data.get("CRYPT_URL_RE", [])

        if "paths" in config_data["TEST_DATA"]:
            path_errors = check_paths_present(config_data)
            if path_errors:
                return errors + path_errors

            if "valid" in config_data["TEST_DATA"]["paths"]:
                domain_errors = check_domains_present(config_data)
                if domain_errors:
                    return errors + domain_errors

                for domain, (path_i, path) in product(config_data["TEST_DATA"]["valid_domains"],
                                                      enumerate(config_data["TEST_DATA"]["paths"]["valid"])):
                    if not is_match(proxy_urls, str(URLBuilder(domain)[path])):
                        errors.append(Invalid(u"Ссылка {} запрещена правилами проксирования".format(URLBuilder(domain)[path]),
                                              path=["TEST_DATA", "paths", "valid", path_i]))

            if "invalid" in config_data["TEST_DATA"]["paths"]:
                domain_errors = check_domains_present(config_data)
                if domain_errors:
                    return errors + domain_errors

                for domain, (path_i, path) in product(config_data["TEST_DATA"]["valid_domains"],
                                                      enumerate(config_data["TEST_DATA"]["paths"]["invalid"])):
                    if is_match(proxy_urls, str(URLBuilder(domain)[path])):
                        errors.append(Invalid(u"Ссылка {} не должна быть разрешена правилами проксирования".format(URLBuilder(domain)[path]),
                                              path=["TEST_DATA", "paths", "invalid", path_i]))

        else:
            if "valid_domains" in config_data["TEST_DATA"]:
                for i, domain in enumerate(config_data["TEST_DATA"]["valid_domains"]):
                    if not is_match(proxy_urls, domain):
                        errors.append(Invalid(u"Домен {} запрещен правилами проксирования".format(domain),
                                              path=["TEST_DATA", "valid_domains", i]))

            if "invalid_domains" in config_data["TEST_DATA"]:
                for i, domain in enumerate(config_data["TEST_DATA"]["invalid_domains"]):
                    if is_match(proxy_urls, domain):
                        errors.append(Invalid(u"Домен {} не должен быть разрешен правилами проксирования".format(domain),
                                              path=["TEST_DATA", "invalid_domains", i]))

        return errors

    def is_test_data_ok(config_data):
        return is_schema_ok(Schema({Required("TEST_DATA"): TEST_DATA_VALIDATOR}, extra=ALLOW_EXTRA), config_data)

    if "TEST_DATA" not in config_data:
        return config_data

    # do not make complex checks on test data if simple one was broken
    if not is_test_data_ok(config_data):
        return config_data

    errors = get_errors(config_data)
    if errors:
        raise MultipleInvalid(errors=errors)
    return config_data


def check_tokens_on_create(service_id):
    """
    Complex check, that validates config vs service id. Also checks expired tokens
    :return: function that returns config if everything is ok and raises MultipleInvalid if there were errors
    """
    def validate(config_data):
        def is_tokens_ok(config_data):
            return is_schema_ok(Schema({Required("PARTNER_TOKENS"): TOKENS_VALIDATOR}, extra=ALLOW_EXTRA), config_data)

        # do not make complex checks on tokens if simple one was broken
        if not is_tokens_ok(config_data):
            return config_data

        errors = []
        for i, token in enumerate(config_data["PARTNER_TOKENS"]):
            if token in HARDCODED_TOKENS:
                if HARDCODED_TOKENS[token] != service_id:
                    errors.append(Invalid(u"Ключ доступа не соответствует сервису ({})".format(service_id), path=["PARTNER_TOKENS", i]))
                # TODO: think about fixed token expiration time
                continue
            try:
                payload = jwt.decode(token, flask.current_app.config["PUBLIC_CRYPT_KEY"], algorithms="RS256")
            except ExpiredSignatureError:
                # don't check expired token
                continue
            except InvalidSignatureError:
                errors.append(Invalid(u"Невалидный ключ. Ошибка проверки подписи", path=["PARTNER_TOKENS", i]))
                continue
            except DecodeError:
                errors.append(Invalid(u"Не удалось прочитать ключ. Попробуйте сгенерировать другой ключ", path=["PARTNER_TOKENS", i]))
                continue
            except InvalidTokenError:
                errors.append(Invalid(u"Невалидный ключ. Попробуйте сгенерировать другой ключ", path=["PARTNER_TOKENS", i]))
                continue

            if payload["sub"] != service_id:
                errors.append(Invalid(u"Ключ доступа не соответствует сервису ({})".format(service_id),
                              path=["PARTNER_TOKENS", i]))
                continue

        if errors:
            raise MultipleInvalid(errors=errors)
        return config_data

    return exception_to_invalid(validate)


def check_tokens_on_activate(old_active_data):
    """
    Because only on activation we have old active config and could check both on forbiden changes.
    Forbids token removing and exchanging.
    (removal and exchange is possible but not in one operation. You should add a new token and then remove the old one)
    :param old_active_data:
    :return:
    """
    def validate(config_data):

        deleted = []
        for old_token in old_active_data["PARTNER_TOKENS"]:
            if old_token not in config_data["PARTNER_TOKENS"]:
                deleted.append(old_token)

        if len(deleted) == len(old_active_data["PARTNER_TOKENS"]):
            raise Invalid(u"Все ранее используемые токены не могут быть удалены", path=["PARTNER_TOKENS"])

        return config_data
    return validate


def is_intersection_exps(exp_1, exp_2):
    if exp_1["EXPERIMENT_TYPE"] == Experiments.NONE or exp_2["EXPERIMENT_TYPE"] == Experiments.NONE:
        return False
    periods = {"exp_1": [], "exp_2": []}

    # построим периоды повторений каждого экспа
    is_both_exps_one_time = not exp_1.get("EXPERIMENT_DAYS", []) and not exp_2.get("EXPERIMENT_DAYS", [])

    for i, exp in (("exp_1", exp_1), ("exp_2", exp_2)):
        start = datetime.strptime(exp["EXPERIMENT_START"], EXPERIMENT_DATETIME_FORMAT)
        if is_both_exps_one_time:
            # оба эксперимента без повторов
            end = start + timedelta(hours=exp["EXPERIMENT_DURATION"])
            periods[i].append((start, end))
        else:
            # хотя бы один эксперимент с повторами
            # нужно привести все даты к текущей неделе
            experiment_days = exp.get("EXPERIMENT_DAYS") or [start.weekday()]
            for day in experiment_days:
                # для нормализации возьмем неделю с 2020-01-06
                start = start.replace(year=2020, month=1, day=6 + day)
                end = start + timedelta(hours=exp["EXPERIMENT_DURATION"])
                periods[i].append((start, end))

    for period_1 in periods["exp_1"]:
        for period_2 in periods["exp_2"]:
            if period_2[0] <= period_1[0] <= period_2[1] or period_2[0] <= period_1[1] <= period_2[1]:
                return True

    return False


def check_experiment_data():

    def validate(config_data):
        errors = []
        # validate data
        for i, exp in enumerate(config_data.get("EXPERIMENTS", [])):
            if not is_schema_ok(Schema({
                Required("EXPERIMENT_TYPE"): Any(*Experiments.all()),
                Required("EXPERIMENT_PERCENT"): All(int, Range(min=1, max=100, msg=u"Поле должно содержать целое число от 1 до 100")),
                Required("EXPERIMENT_START"): validate_ts,
                Required("EXPERIMENT_DURATION"): All(int, Range(min=1, msg=u"Поле должно содержать целое число не меньше 1")),
                Optional("EXPERIMENT_DAYS", default=[]): [All(int, Range(min=0, max=6, msg=u"Поле должно содержать целое число от 0 до 6"))],
                Required("EXPERIMENT_DEVICE"): All(Length(min=1, max=2), [Any(*UserDevice.all())]),
            }), exp):
                errors.append(Invalid(u"Некорректные данные", path=["EXPERIMENTS", i]))
        if errors:
            raise MultipleInvalid(errors=errors)

        intersections = set()
        # validate experiments intersections
        for i, exp in enumerate(config_data.get("EXPERIMENTS", [])):
            for j, another_exp in enumerate(config_data.get("EXPERIMENTS", [])):
                if exp is another_exp:
                    continue
                if is_intersection_exps(exp, another_exp):
                    intersections.add(tuple(sorted((i, j))))
        for exps in intersections:
            for ind in exps:
                errors.append(Invalid(u"Эксперименты пересекаются", path=["EXPERIMENTS", ind]))

        if errors:
            raise MultipleInvalid(errors=errors)
        return config_data

    return validate
