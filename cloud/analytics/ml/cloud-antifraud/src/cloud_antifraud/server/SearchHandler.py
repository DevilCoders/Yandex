import tornado.web
import pandas as pd
import logging
import ujson
import numpy as np
logger=logging.getLogger(__name__)

class SearchHandler(tornado.web.RequestHandler):

    def set_default_headers(self):
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')


    def post(self):
        payload =  ujson.loads(self.request.body)
        search_str = payload['account_name']
        inds = np.where(self.application.features_dash.account_name.str.lower().str.contains(search_str.lower()) 
            | self.application.features_dash.billing_account_id.str.lower().str.contains(search_str.lower()) 
        )[0].tolist()
        
        self.write({'inds': inds})
