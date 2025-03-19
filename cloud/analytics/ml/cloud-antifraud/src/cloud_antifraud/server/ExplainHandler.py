import tornado.web
import pandas as pd
import logging
from datetime import datetime
logger=logging.getLogger(__name__)

class ExplainHandler(tornado.web.RequestHandler):
   
    def _predict_fn(self, x):
            d = pd.DataFrame(x, columns= self.application.problem.X.columns)
            t = self.application.problem.preprocessor.transform(d)               
            return self.application.clf.predict_proba(t)
    
  

    def get(self):

        cloud_id = self.get_argument('cloud_id')
        billing_account_id = self.get_argument('billing_account_id')
        account_name = self.get_argument('account_name')
        df = self.application.features_dash


        sample = df[(df.cloud_id == cloud_id)  & (df.billing_account_id == billing_account_id)].copy()
        sampleT = sample.T.reset_index()

        sampleT.columns = ['Property Name', 'Property Value']
        # for col in ['time', 'ba_time', 'trial_start']:
        #     sample[col] = sample[col].apply(datetime.utcfromtimestamp)


        logger.debug(f"Explaining {(billing_account_id, cloud_id)}")
       


        logger.debug((billing_account_id, cloud_id))
        cols = self.application.problem.X.columns
        sample_train = df[cols].iloc[((df.cloud_id == cloud_id) \
                & (df.billing_account_id == billing_account_id)).values, :].values.reshape(-1)
        exp = self.application.explainer.explain_instance(sample_train, self._predict_fn, num_features=20)

        shap_df = pd.DataFrame({
                        'Property Name': cols.values.tolist(), 
                        'SHAP': self.application.shap_explainer.shap_values(
                                sample_train.reshape(1,-1))[0].reshape(-1).astype('str').tolist()
                    })

        
        sample_feature_imortance_df = sampleT.merge(shap_df,  on='Property Name', how='left').fillna('N/A')

        self.write({"explanation": pd.DataFrame(exp.as_list(), columns=['x', 'y']).to_dict(orient='list'),
                    "sample" : sample_feature_imortance_df.to_dict(orient='list'),
                    'confidence':sample['conf1'].values[0]
        })

        

        
        
