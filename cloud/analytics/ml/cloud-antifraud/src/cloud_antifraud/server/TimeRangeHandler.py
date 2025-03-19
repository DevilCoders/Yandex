import tornado.web
import pandas as pd
import logging
import ujson

logger=logging.getLogger(__name__)

class TimeRangeHandler(tornado.web.RequestHandler):
   

    def get(self):
        df = self.application.features_dash
        response = {'min_time': int(df.ba_time.min()), 'max_time': int(df.ba_time.max())}
        logger.debug(response)
        self.write(response)