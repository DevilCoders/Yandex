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

class RulesHeatmapHandler(tornado.web.RequestHandler):

   

    def iou_matrix(self, created_rules, df):
        iou_created_rules = np.zeros((len(created_rules), len(created_rules)))
        flat_iou = []
        for idx1, col1 in enumerate(created_rules):
            for idx2, col2 in enumerate(created_rules):
                if idx1 > idx2:
                    col1_set = set(np.where(df[col1] ==1)[0].tolist())
                    col2_set = set(np.where(df[col2] ==1)[0].tolist())
                    intersection = len(col1_set.intersection(col2_set))
                    union =  len(col1_set.union(col2_set))
                    iou = 0 if union ==0 else intersection / union
                    iou_created_rules[idx1, idx2] = iou
                    iou_created_rules[idx2, idx1] = iou
                    flat_iou.append(iou)
        return iou_created_rules, flat_iou

    def get(self):
        rules = self.get_argument('rules', None)
        df = self.application.features_dash
        created_rules = [col for col in df.columns if ((rules in col) and (f'{rules}s' not in col))]
        date_from_str = self.get_argument('from', None)
        date_to_str = self.get_argument('to', None)
        if (date_from_str is not None) and (date_to_str is not None):
            date_from=int(date_from_str)
            date_to=int(date_to_str)
            df = df[(df.ba_time >= date_from) & (df.ba_time <= date_to)]
        

        iou_created_rules, flat_iou = self.iou_matrix(created_rules, df)

        self.write({'iou':iou_created_rules.tolist()}) 
       
