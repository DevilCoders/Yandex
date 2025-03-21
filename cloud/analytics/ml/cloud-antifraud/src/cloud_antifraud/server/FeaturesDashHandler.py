import tornado.web
import pandas as pd
import logging
import ujson

logger=logging.getLogger(__name__)

class FeaturesDashHandler(tornado.web.RequestHandler):
   

    def set_default_headers(self):
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')

    def post(self):
        df = self.application.features_dash[self.application.ui_cols + ['y', 'ba_time']]
        self.write(df.to_dict(orient='list'))