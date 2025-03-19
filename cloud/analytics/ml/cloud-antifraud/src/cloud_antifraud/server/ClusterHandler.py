import tornado.web
import pandas as pd
import logging
import ujson

logger=logging.getLogger(__name__)

class ClusterHandler(tornado.web.RequestHandler):
    def initialize(self, explainer, problem, features_dash):
        self._explainer = explainer
        self._problem = problem
        self._features_dash = features_dash


    def post(self):
        payload =  ujson.loads(self.request.body)
        ids = payload['accounts']
        df = self._features_dash[self._problem.X.columns.tolist() + ['y_back', 'is_target']]
        if len(ids) > 5:
            in_cluster = df.iloc[ids, :]
            std_in_cluster = df[self._problem.X.columns].iloc[ids, :].std()
            std_all = df[self._problem.X.columns].std()
            std_ratio = std_in_cluster / std_all
            std_ratio = std_ratio.sort_values().to_frame()
            logger.debug("Explaining cluster")
            self.render('templates/cluster.html', sample=in_cluster.describe().to_html(classes='sample'), 
                        std_ratio = std_ratio.to_html(classes='std_ratio'))
