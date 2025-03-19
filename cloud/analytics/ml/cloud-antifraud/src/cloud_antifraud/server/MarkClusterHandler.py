import tornado.web
import pandas as pd
import logging
import ujson
import numpy as np
logger=logging.getLogger(__name__)

class MarkClusterHandler(tornado.web.RequestHandler):
    def initialize(self, manual_labels_path):
        self._manual_labels_path = manual_labels_path


    def post(self):
        payload =  ujson.loads(self.request.body)
        labels_df  = pd.DataFrame(payload)
        labels_df['manual_y'] = labels_df['manual_y'].astype(int)
        labels_df.to_pickle(self._manual_labels_path)
        self.write({'status': 'ok'})
