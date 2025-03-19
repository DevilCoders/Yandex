import os
import sys
sys.path.append(os.path.abspath(f"{os.environ['SOURCE_CODE_PATH']}/src"))

import pickle
import pandas as pd
import logging.config
import yt.wrapper as yt
from datetime import datetime
import sklearn
from sklearn.preprocessing import MinMaxScaler
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from ba_trust.utils import Preprocessor, ModelPipeline, LogRegressionWrapper, training_cols

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

output_path = os.environ['TMP_OUTPUT_PATH']
logger.debug(output_path)
yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
yt.config["token"] = os.environ['YT_TOKEN']


def main():
    yt_adapter = YTAdapter(token=os.environ['YT_TOKEN'])
    logger.debug(sklearn.__version__)
    train = pd.DataFrame(yt.read_table("//home/cloud_analytics/ml/ba_trust/train"))
    model = ModelPipeline([('preprocessor', Preprocessor()), ('scaler', MinMaxScaler()), ('log_reg', LogRegressionWrapper())])
    model.fit(train[training_cols], train['target_dbt'])

    dmp = pickle.dumps(model)
    yt_adapter.yt.write_file(f"//home/cloud_analytics/ml/ba_trust/model/model_{datetime.now().strftime('%Y-%m-%d')}.pkl", dmp)
    yt_adapter.optimize_chunk_number("//home/cloud_analytics/ml/ba_trust/trust_scores_history", retries_num=3)

if __name__ == "__main__":
    main()
