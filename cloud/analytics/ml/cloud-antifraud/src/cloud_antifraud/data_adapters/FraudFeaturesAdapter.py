from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.cache import cached
from clan_tools.utils.time import utcms2datetime
import os
from matplotlib import pyplot as plt
import numpy as np
from sklearn.ensemble import IsolationForest
from sklearn.model_selection import train_test_split
import regex
import re
from collections import Counter 
import numpy as np
import pandas as pd
from numba import njit, prange
from sklearn.cluster import DBSCAN
import json
from datetime import datetime

from clan_tools.utils.timing import timing


class FraudFeaturesAdapter:
    def __init__(self, ch_adapter:ClickHouseYTAdapter, features_table:str):
        self._ch_adapter = ch_adapter
        self._features_table = features_table

    @timing
    def get_data(self):
        features = self._ch_adapter.execute_query(f''' 
                --sql
select * from 
(select * from 
(select * from "//home/cloud_analytics/tmp/antifraud/{self._features_table}" as f
inner join (select billing_account_id, ba_state, 
                (is_fraud or block_reason='mining') as is_target,
                unblock_reason,
                is_verified, 
                ba_person_type='company' as company,
                ba_name,
                balance,
                sales_name,
                first_name,
                last_name,
                sku_lazy
            from "//home/cloud_analytics/cubes/acquisition_cube/cube"
            where event='ba_created' 
)  acq using billing_account_id ) as acq_pass
inner join (select billing_account_id, 
                    sum(trial_consumption) as trial_consumption,
                    sum(real_consumption) as real_consumption,
                    sum(if(balance>0, balance, 0)) as total_balance
            from "//home/cloud_analytics/cubes/acquisition_cube/cube"
            where event='day_use' 
            group by billing_account_id
) as agg_cons using billing_account_id) as features
inner join (select passport_uid, is_bad as is_bad_current
            from  "//home/cloud_analytics/tmp/antifraud/current_rules_state") as cr
on features.passport_id = toUInt64(cr.passport_uid)
            --endsql
            '''
                , 
            to_pandas=True)
        rules_cols = ['created_rules', 'cloud_rules', 'suspicious_rules', 'recalc_rules']
        features[rules_cols] = features[rules_cols].fillna(value='')
        features[rules_cols] = features[rules_cols].astype(str)
        features.fillna(0, inplace=True)
        return features