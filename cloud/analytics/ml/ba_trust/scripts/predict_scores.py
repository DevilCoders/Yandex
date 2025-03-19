import os
import sys
sys.path.append(os.path.abspath(f"{os.environ['SOURCE_CODE_PATH']}/src"))
# sys.path.append(os.path.abspath("/home/albina-volk/arc/arcadia/cloud/analytics/ml/ba_trust/src"))
import pickle
import numpy as np
import logging.config
from datetime import datetime
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from ba_trust.utils import training_cols

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def normalize(array: np.array, th: float = 0.15):
    x = np.log(array[array <= th] + 1e-10)
    x = (x > -7) * (x + 7)
    x = 1 - x/x.max()/2

    y = - np.log(array[array > th])
    y = y/y.max()/2

    result = np.zeros(len(array))
    result[array <= th] = x
    result[array > th] = y
    return np.abs(result)


def main():
    yt_adapter = YTAdapter(token=os.environ['YT_TOKEN'])

    test = yt_adapter.read_table("//home/cloud_analytics/ml/ba_trust/test")
    assert(len(test) > 0)

    test['person_type'] = test['person_type'].fillna('individual')
    ba = test.sort_values('date')['billing_account_id']
    test = test.sort_values('date').groupby('billing_account_id').ffill()
    test['billing_account_id'] = ba
    test = test[test['date'] == test['date'].max()]
    logger.debug(len(test[test.isna().sum(axis=1) > 0]))
    # assert(len(test[test.isna().sum(axis=1) > 0]) < 1000)

    last_model_name = max(yt_adapter.yt.list("//home/cloud_analytics/ml/ba_trust/model"))
    model_file = yt_adapter.yt.read_file(f"//home/cloud_analytics/ml/ba_trust/model/{last_model_name}").read()
    model = pickle.loads(model_file)
    test['scores'] = model.predict(test[training_cols])

    result = test[['billing_account_id', 'scores']]
    result.loc[(test['state'] == 'suspended') | (test['state'] == 'inactive'), 'scores'] = 1
    result['date'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    yt_schema = [{'name': 'billing_account_id', 'type': 'string'}, {'name': 'scores', 'type': 'double'}, {'name': 'date', 'type': 'string'}]
    logger.debug(f"yt_schema: {yt_schema}")
    logger.debug(result.head())
    yt_adapter.save_result("//home/cloud_analytics/ml/ba_trust/trust_scores_history", yt_schema, result, append=True)
    result_normalized=result.copy()
    result_normalized['scores'] = normalize(result_normalized['scores'].values)

    yt_adapter.save_result("//home/cloud_analytics/ml/ba_trust/trust_scores", yt_schema, result_normalized, append=False)

if __name__ == "__main__":
    main()
