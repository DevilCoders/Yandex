import tornado.web
import pandas as pd
import logging
import ujson
import numpy as np
logger=logging.getLogger(__name__)

class FeaturesFilterHandler(tornado.web.RequestHandler):
 

   

    def get(self):
        df = self.application.features_dash[self.application.ui_cols + ['y', 'ba_time']]
        date_from = int(self.get_argument('from'))
        date_to = int(self.get_argument('to'))
        self.write({'indices': np.where((df.ba_time >= date_from ) & (df.ba_time <= date_to ))[0].tolist() })