# pylint: disable=no-value-for-parameter
import os

from clan_tools.data_adapters.YTAdapter import YTAdapter
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
print(os.listdir('.'))
from spyt import spark_session
import pytz
import logging.config
from datetime import datetime

from consumption_predictor.data_adapters.SparkFeaturesAdapter import SparkFeaturesAdapter
from consumption_predictor.feature_extraction.SparkFeaturesExtractor import SparkFeaturesExtractor
import spyt
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from clan_tools.logging.logger import default_log_config
logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)

import click





@click.command('extract_features')
@click.option('--train_features_dir')
@click.option('--predict_features_dir')
@click.option('--write', is_flag=True)
def extract_features(train_features_dir:str, predict_features_dir:str, write:bool):

    
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        spyt.info(spark)
        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])

        features_adapter = SparkFeaturesAdapter(spark, yt_adapter)

        features_conf = dict(paid_cons_started = 1,
                            target_cons = 50000,
                            hist_period = 15,
                            pred_period = 1)
        features_extractor = SparkFeaturesExtractor(features_adapter, **features_conf)


        spark_features = features_extractor.get_features().cache()

        table_name = datetime.now(tz=pytz.utc).strftime('%Y-%m-%dT%H:%M:%S')
        spark_features.write.optimize_for("scan").yt(f'{train_features_dir}/{table_name}')

        spark_predict_features = features_extractor.pred_features(spark_features)
        spark_predict_features.write.optimize_for("scan").yt(f'{predict_features_dir}/{table_name}')

    

if __name__ == '__main__':
    extract_features()
