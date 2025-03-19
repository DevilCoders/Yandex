import itertools
import pickle
from os.path import join
import luigi
import math
import numpy as np
import pandas as pd
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from textwrap import dedent
from clan_tools import utils
from os.path import join
import logging
from cloud_antifraud.tasks.PassportRules import PassportRules
import tornado

logger = logging.getLogger(__name__)

class RulesBarHandler(tornado.web.RequestHandler):

   

    def get(self):
        rules = self.get_argument('rules', None)
        df = self.application.features_dash
        created_rules = [col for col in df.columns if ((rules in col) and (f'{rules}s' not in col))]
        created_rules = [f'{rules}{i}' for i in range(len(created_rules))]
        date_from_str = self.get_argument('from', None)
        date_to_str = self.get_argument('to', None)
        if (date_from_str is not None) and (date_to_str is not None):
            date_from=int(date_from_str)
            date_to=int(date_to_str)
            df = df[(df.ba_time >= date_from) & (df.ba_time <= date_to)]
        
        detected_list = []
        false_positive_list = []
        # active = []
        for rule in created_rules:
            n_false_positive = np.nansum((df[rule] ==1) & 
                                    (df.is_verified 
                                    | (df.unblock_reason=='manual')))
            n_detected = np.nansum(df[rule]==1) - n_false_positive

            detected_list.append(int(n_detected))
            false_positive_list.append(int(n_false_positive))

        self.write({'rules': created_rules, 'detected': detected_list, 'false_positives':false_positive_list}) 
       
