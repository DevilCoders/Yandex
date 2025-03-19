# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import pickle
import pandas as pd
from datetime import datetime
from os.path import join as path_join
from sklearn.isotonic import IsotonicRegression
from sklearn.metrics import roc_auc_score, precision_score, recall_score, f1_score
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import lit
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.timing import timing
from consumption_predictor_v2.utils.helpers_spark import BA_ID_COL_NAME, BR_MSK_DT_NAME, Y_TRUE_COL_NAME, Y_PRED_COL_NAME
from consumption_predictor_v2.utils.helpers_spark import read_preprocess_catboost_result_spark
from consumption_predictor_v2.train_config.csm_params import PARAMS

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

CSM_TARGET_COL_NAME = 'csm_target'
CSM_RESPONSE_COL_NAME = 'csm_proba'


@timing
def calculate_csm_metrics(df, n_period=10):
    brd = PARAMS['PRED_PROBA_BRD']
    p_val = 1-PARAMS['CONF_INTERVAL']
    df_metrics = []
    all_dates = sorted(df[BR_MSK_DT_NAME].unique())
    chunked_dates = [all_dates[i:i+n_period] for i in range(0, len(all_dates), n_period)]
    for dt_chunk in chunked_dates:
        tdf = df[df[BR_MSK_DT_NAME].isin(dt_chunk)].copy()
        if tdf[CSM_TARGET_COL_NAME].nunique()==2:
            metrics = {}
            metrics.update({'Gini_rsp': roc_auc_score(tdf[CSM_TARGET_COL_NAME], tdf[CSM_RESPONSE_COL_NAME])*2-1})
            metrics.update({'Gini_tgt': roc_auc_score(tdf[CSM_TARGET_COL_NAME], tdf[CSM_RESPONSE_COL_NAME]>brd)*2-1})
            metrics.update({'Precision': precision_score(tdf[CSM_TARGET_COL_NAME], tdf[CSM_RESPONSE_COL_NAME]>brd)})
            metrics.update({'Recall': recall_score(tdf[CSM_TARGET_COL_NAME], tdf[CSM_RESPONSE_COL_NAME]>brd)})
            metrics.update({'F1_Score': f1_score(tdf[CSM_TARGET_COL_NAME], tdf[CSM_RESPONSE_COL_NAME]>brd)})
            df_metrics.append(metrics)
    df_metrics = pd.DataFrame(df_metrics)

    res = {}
    res.update({f'{key}_mean': val for key, val in dict(df_metrics.mean()).items()})
    res.update({f'{key}_std': val for key, val in dict(df_metrics.std()).items()})
    res.update({f'{key}_ci_lower': val for key, val in dict(df_metrics.quantile(q=p_val/2)).items()})
    res.update({f'{key}_ci_upper': val for key, val in dict(df_metrics.quantile(q=1-p_val/2)).items()})
    return res


@click.command()
@click.option('--metrics_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics/train_csm_metrics")
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--samples_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_train_samples")
@click.option('--models_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/models_history")
@click.option('--calibrators_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/calibrators_history/csm")
def csm_eval_metrics_prod(metrics_path: str, features_path: str, samples_path: str, models_path: str, calibrators_path: str):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        logger.info('Loading and preparing datasets...')
        last_model_name = max(yt_adapter.yt.list(models_path))
        last_model_dt = datetime.strptime(last_model_name, 'model_%Y%m%d_%H%M.bin')

        spdf_features = spark.read.yt(features_path)

        def read_and_format_result_table(result_table_name):
            pd_res = (
                read_preprocess_catboost_result_spark(spark, path_join(samples_path, result_table_name))
                .join(spdf_features, on=[BA_ID_COL_NAME, BR_MSK_DT_NAME], how='left')
                .select(BA_ID_COL_NAME, BR_MSK_DT_NAME, Y_TRUE_COL_NAME, Y_PRED_COL_NAME,
                        F.coalesce('billing_record_total_rub', lit(0)).alias('current_1d_cons'),
                        F.coalesce('prev_15d_cons', lit(0)).alias('prev_15d_cons'),
                        F.coalesce('prev_30d_cons', lit(0)).alias('prev_30d_cons'))
                .toPandas()
            )
            return pd_res

        df_train_val = read_and_format_result_table('result_train_val')
        df_test = read_and_format_result_table('result_test')
        df_oot = read_and_format_result_table('result_oot')

    csm_cond = PARAMS['TARGET_PAID_COND']
    target_calc_cols = ['prev_15d_cons', 'current_1d_cons', Y_TRUE_COL_NAME]
    df_train_val[CSM_TARGET_COL_NAME] = (df_train_val[target_calc_cols].sum(axis=1)>=csm_cond).astype(int)
    df_test[CSM_TARGET_COL_NAME] = (df_test[target_calc_cols].sum(axis=1)>=csm_cond).astype(int)
    df_oot[CSM_TARGET_COL_NAME] = (df_oot[target_calc_cols].sum(axis=1)>=csm_cond).astype(int)

    tr_df_train_val = df_train_val[df_train_val['prev_30d_cons']<csm_cond].copy()
    tr_df_train_val = tr_df_train_val[tr_df_train_val['prev_30d_cons']>PARAMS['MIN_PAID_LAST_30D']].reset_index(drop=True)
    tr_df_test = df_test[df_test['prev_30d_cons']<csm_cond].copy()
    tr_df_test = tr_df_test[tr_df_test['prev_30d_cons']>PARAMS['MIN_PAID_LAST_30D']].reset_index(drop=True)
    tr_df_oot = df_oot[df_oot['prev_30d_cons']<csm_cond].copy()
    tr_df_oot = tr_df_oot[tr_df_oot['prev_30d_cons']>PARAMS['MIN_PAID_LAST_30D']].reset_index(drop=True)

    logger.info('Calibration fitting and saving calibrator...')
    calibr_csm = IsotonicRegression(out_of_bounds='clip', y_min=PARAMS['MIN_CALIBR_PROBA'], y_max=PARAMS['MAX_CALIBR_PROBA'])
    calibr_csm.fit(df_train_val[Y_PRED_COL_NAME], df_train_val[CSM_TARGET_COL_NAME])

    pred_calc_cols = ['prev_15d_cons', 'current_1d_cons', Y_PRED_COL_NAME]
    tr_df_train_val[CSM_RESPONSE_COL_NAME] = calibr_csm.predict(tr_df_train_val[pred_calc_cols].sum(axis=1))
    tr_df_test[CSM_RESPONSE_COL_NAME] = calibr_csm.predict(tr_df_test[pred_calc_cols].sum(axis=1))
    tr_df_oot[CSM_RESPONSE_COL_NAME] = calibr_csm.predict(tr_df_oot[pred_calc_cols].sum(axis=1))

    calibr_csm_ser = pickle.dumps(calibr_csm)
    corr_calibr_name = last_model_dt.strftime('csm_calibr_%Y%m%d_%H%M.pkl')
    yt_adapter.yt.write_file(f'{calibrators_path}/{corr_calibr_name}', calibr_csm_ser)

    logger.info('Caclulating CSM metrics...')
    res_df = []
    for dataset, df in [('train_val', tr_df_train_val), ('test', tr_df_test), ('oot', tr_df_oot)]:
        row = {
            'updated': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'model_name': 'Cons_predictor-train-csm',
            'dataset_name': dataset
        }
        key_metrics = calculate_csm_metrics(df)
        row.update(key_metrics)
        res_df.append(row)
    res_df = pd.DataFrame(res_df)

    logger.info('Saving metrics...')
    yt_schema = yt_adapter.get_pandas_default_schema(res_df)
    yt_adapter.save_result(metrics_path, yt_schema, res_df)
    yt_adapter.optimize_chunk_number(metrics_path)


if __name__ == '__main__':
    csm_eval_metrics_prod()
