#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import os, sys, pandas as pd, datetime, telebot
if module_path not in sys.path:
    sys.path.append(module_path)
from data_loader import clickhouse
from global_variables import (
    metrika_clickhouse_param_dict,
    cloud_clickhouse_param_dict
)
from init_variables import queries,queries_append_mode
from vault_client import instances

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)

@vh.lazy(
    object,
    mysql_token=vh.Secret,
    yt_token=vh.Secret,
)
