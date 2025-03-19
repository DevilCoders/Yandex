import itertools
import pickle
from os.path import join
import luigi
import math
import numpy as np
import pandas as pd
from clan_tools.utils.path import childfile2str
from textwrap import dedent
from clan_tools import utils
from os.path import join
import logging
from clan_tools.utils.conf import read_conf
from cloud_antifraud.data_adapters.FraudFeaturesAdapter import FraudFeaturesAdapter
from cloud_antifraud.feature_extraction.FeatureExtractor import FeatureExtractor
from cloud_antifraud.ml.MLProblem import  MLProblemCreator
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
import json
from cloud_antifraud.ml.FraudAnalyzer import FraudAnalyzer
import logging.config
import numpy as np 
import pickle
import dill
from clan_tools.data_adapters.YTAdapter import YTAdapter
from cloud_antifraud.tasks.NetworkPortsConnections import NetworkPortsConnections
from cloud_antifraud.tasks.CurrentRulesState import CurrentRulesState
from cloud_antifraud.tasks.PassportRules import PassportRules
import shap

logger = logging.getLogger(__name__)

class AnalyzeFraud(luigi.Task):

    def requires(self):
        return [NetworkPortsConnections(), CurrentRulesState()]


    

    def run(self):
        features = FraudFeaturesAdapter(ClickHouseYTAdapter(), 'network_ports_connections').get_data()
        features = FeatureExtractor().extract_features(features)
        exclude_cols = ['billing_account_id', 'account_name', 'cloud_id',  'passport_id',
                        'is_verified',  'phone', 'cloud_rules', 'created_rules', 'suspicious_rules',
                        'ba_time', 'ba_name', 'is_target',  'ba_state', 'balance', 'sku_lazy', 'total_balance',
                        'real_consumption', 'trial_consumption', 'recalc_rules', 
                        'is_suspicious', 'is_bad', 'is_bad_current',
                        'time', 'first_name', 'last_name', 'phone_oper_code_str',
                        'passport_id',
                        'puid_threshold',
                        'is_ipv4',
                        'registration_ip',
                        'registration_subnet',
                        'registration_city',
                        'registration_region',
                        'registration_district',
                        'registration_isp',
                        'registration_asn',
                        'registration_org',
                        'company',
                        'account_name_is_company',
                        'ba_name_is_company',
                        'acc_name_first',
                        'acc_name_last',
                        'time',
                        'ba_time',
                        'ba_time_10min',
                        'sales_name',
                        'trial_start',
                        'trial_start_10min',
                        'unblock_reason'
                        ]

       

        problem = MLProblemCreator(exclude_columns=exclude_cols, target_column='is_target')\
            .get_problem(features)
        features_dash, clf, explainer, shap_explainer = FraudAnalyzer(problem).analyze()


        





        with open(self.output()['features'].path, 'wb') as output:
            pickle.dump([features_dash, clf, problem, shap_explainer], output)

        with open(self.output()['explainer'].path, 'wb') as output:
            dill.dump(explainer, output)

        yt_adapter=YTAdapter()
        with open(self.output()['features'].path, 'rb') as f:
            yt_path = '//home/cloud_analytics/tmp/antifraud/features.pkl'
            yt_adapter.yt.create(type="file", path=yt_path, recursive=True, force=True)
            yt_adapter.yt.write_file(yt_path, f)
      
        with open(self.output()['explainer'].path, 'rb') as f:
            yt_path = '//home/cloud_analytics/tmp/antifraud/explainer.dill'
            yt_adapter.yt.create(type="file", path=yt_path, recursive=True, force=True)
            yt_adapter.yt.write_file(yt_path, f)
      
       

    def output(self):
        return  {'features': luigi.LocalTarget(join('data', 'cache', 'features.pkl')),
                  'explainer': luigi.LocalTarget(join('data', 'cache', 'explainer.dill'))}
