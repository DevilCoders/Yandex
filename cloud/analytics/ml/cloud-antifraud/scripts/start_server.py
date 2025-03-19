from datetime import datetime
import pickle
from xml.sax.handler import feature_string_interning
from pandas.tests.extension.base import dtype
import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web
import click
from tornado.options import define, options
import dill
import logging.config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.conf import read_conf
import pandas as pd
from cloud_antifraud.server.MainHandler import MainHandler
from cloud_antifraud.server.FeaturesDashHandler import FeaturesDashHandler
from cloud_antifraud.server.FeaturesFilterHandler import FeaturesFilterHandler
from cloud_antifraud.server.ExplainHandler import ExplainHandler
from cloud_antifraud.server.RulesBarHandler import RulesBarHandler
from cloud_antifraud.server.RulesHeatmapHandler import RulesHeatmapHandler
from cloud_antifraud.server.SearchHandler import SearchHandler
from clan_tools.utils.time import utcms2datetime
import os

from cloud_antifraud.server.TimeRangeHandler import TimeRangeHandler


# import os


logging.config.dictConfig(read_conf('config/logger.yml'))
logger = logging.getLogger(__name__)

yt_adapter = YTAdapter()


def download_data():

    with open('data/cache/features.pkl', 'wb') as file:
        fetures_yt = yt_adapter.yt.read_file('//home/cloud_analytics/tmp/antifraud/features.pkl')
        fetures_yt_content = fetures_yt.read()
        file.write(fetures_yt_content)

    with open('data/cache/explainer.dill', 'wb') as file:
        fetures_yt = yt_adapter.yt.read_file('//home/cloud_analytics/tmp/antifraud/explainer.dill')
        fetures_yt_content = fetures_yt.read()
        file.write(fetures_yt_content)


    logger.debug('Loading features')
    with open('data/cache/features.pkl', 'rb') as input:
        features_dash, clf, problem, shap_explainer = pickle.load(input)
        logger.debug(f"Data from {utcms2datetime(features_dash['ba_time'].min()*1000)} to {utcms2datetime(features_dash['ba_time'].max()*1000)}")

    logger.debug('Loading exapliner')
    with open('data/cache/explainer.dill', 'rb') as input:
        explainer = dill.load(input)
    
    ui_cols  = ['account_name', 
                'ba_state',
                'is_target',
                'is_bad',
                'is_bad_current',
                'billing_account_id',
                'cloud_id',
                'passport_id',
                'trial_puid_diff',
                'ba_puid_diff',
                'vm_count',
                'max_cores',
                'trial_consumption',
                'real_consumption',
                "total_balance",
                "sales_name",
                "n_sim_accounts", 
                "same_ip",
                "same_phone",
                "registration_country",     
                'phone',
                'y_back',
                'conf1', 
                'cloud_rules',
                'created_rules',
                'suspicious_rules',
                'is_verified',
                'u0', 
                'u1']

    return features_dash, clf, ui_cols, problem, explainer, shap_explainer


features_dash, clf, ui_cols, problem, explainer, shap_explainer = download_data()

logger.debug('Prepearing data')








class CloudAntifraudApp(tornado.web.Application):
    def __init__(self, features_dash, clf, ui_cols, problem, explainer, shap_explainer):
        handlers=[
            ('/api/explain', ExplainHandler),
             ('/api/search', SearchHandler),
             ('/api/features_dash', FeaturesDashHandler),
             ('/api/time_range', TimeRangeHandler),
             ('/api/features_filter', FeaturesFilterHandler),
             ('/api/rules/heatmap', RulesHeatmapHandler),
             ('/api/rules/bar', RulesBarHandler),
             ]
        
        settings = dict(
             static_path="static/build/static",
        template_path="static/build",
        compress_response=True)

        self.features_dash = features_dash
        self.clf = clf
        self.ui_cols = ui_cols
        self.problem = problem
        self.explainer = explainer
        self.shap_explainer = shap_explainer

        tornado.web.Application.__init__(self, handlers, **settings)
        


@click.command()
@click.option('--port')
def main(port):

    app = CloudAntifraudApp(features_dash, clf, ui_cols, problem, explainer, shap_explainer)

    server = tornado.httpserver.HTTPServer(app)
    server.bind(port)
    server.start(1)  # forks one process per cpu
    some_time_period = 60*60*1000 # 1000 is one second
    def update_data():
        logger.debug(f'Updateing data {os.getpid()}')
        features_dash, clf, ui_cols, problem, explainer = download_data()

        app.features_dash=features_dash
        app.problem=problem
        app.explainer=explainer
        app.shap_explainer=shap_explainer
        app.clf=clf
        app.ui_cols=ui_cols




    tornado.ioloop.PeriodicCallback(update_data, some_time_period).start()
    tornado.ioloop.IOLoop.instance().start()

if __name__ == "__main__":
    main()