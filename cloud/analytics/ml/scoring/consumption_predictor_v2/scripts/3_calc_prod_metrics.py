# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import re
import click
import pickle
from spyt import spark_session
import logging.config
import spyt
import pandas as pd
from datetime import datetime, timedelta
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from os.path import join as path_join
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
from sklearn.metrics import roc_auc_score, precision_score, recall_score, f1_score
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.train_config.onb_params import PARAMS as ONB_PARAMS
from consumption_predictor_v2.train_config.csm_params import PARAMS as CSM_PARAMS

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def load_map_of_calibrators(yt_adapter, yt_path, date_grid):

    def calibr_creation_time(model_name):
        only_numbers_in_name = re.sub(r'[\D]+', '', model_name)
        dt = datetime.strptime(only_numbers_in_name, '%Y%m%d%H%M')
        dt_fmt = dt.strftime('%Y-%m-%d %H:%M:%S')
        return dt_fmt

    # all models in path
    df_models = pd.DataFrame({'calibr_path': yt_adapter.yt.list(yt_path)})
    df_models['creation_time'] = df_models['calibr_path'].apply(calibr_creation_time)
    df_models['x'] = 'x'

    # all dates in date_grid
    df_dates = pd.DataFrame({'rep_date': date_grid})
    df_dates['x'] = 'x'

    # date to model path mapping
    df_res = df_models.merge(df_dates, on=['x'], how='inner')
    df_res = df_res[df_res['creation_time'] < df_res['rep_date']]
    date_creation_keys = df_res.groupby('rep_date')['creation_time'].agg(max).reset_index()
    date_path_df = date_creation_keys.merge(df_models, on='creation_time', how='left')
    date_path_map = date_path_df.set_index('rep_date')['calibr_path'].to_dict()

    # date to model mapping
    date_model_map = {}
    for date, model_name in date_path_map.items():
        calibr_ser = yt_adapter.yt.read_file(path_join(yt_path, model_name)).read()
        calibr = pickle.loads(calibr_ser)
        date_model_map.update({date: calibr})

    return date_model_map


@click.command()
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--target_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_target")
@click.option('--results_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/prod_results")
@click.option('--report_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics")
def calc_prod_metrics(features_path, target_path, results_path, report_path):
    logger.info("Start metrics calculation")
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    prev_cons_columns = ["prev_1d_cons", "prev_7d_cons", "prev_14d_cons", "prev_15d_cons", "prev_21d_cons", "prev_30d_cons", "prev_45d_cons"]

    gen_metrics_path = path_join(report_path, 'predict_gen_metrics')
    onb_metrics_path = path_join(report_path, 'predict_onb_metrics')
    csm_metrics_path = path_join(report_path, 'predict_csm_metrics')
    calibrators_yt = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/calibrators_history'
    onb_calibr_path = path_join(calibrators_yt, 'onboarding')
    csm_calibr_path = path_join(calibrators_yt, 'csm')

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        logger.info("Reading tables...")
        dts_true = spark.read.yt(target_path)
        dts_pred = spark.read.yt(results_path)

        assert dts_true.filter(col('next_14d_cons').isNull()).count() == 0, f"NULL values in '{target_path}'"
        assert dts_pred.filter(col('next_14d_cons_pred').isNull()).count() == 0, f"NULL values in '{results_path}'"

        last_ground_truth_date = dts_true.agg(F.max('billing_record_msk_date')).collect()[0][0]
        metrics_period_start = (pd.to_datetime(last_ground_truth_date) - timedelta(days=14)).strftime('%Y-%m-%d')
        logger.info(f'Metrics calculation period: {metrics_period_start} to {last_ground_truth_date}')

        dts = (
            dts_true
            .join(dts_pred, on=['billing_record_msk_date', 'billing_account_id'], how='inner')
            .filter(col('billing_record_msk_date')>=metrics_period_start)
            .cache()
        )
        logger.info(f'Table size: {dts.count()}')

        df = (
            spark.read.yt(features_path)
            .select(
                "billing_record_msk_date",
                "billing_account_id",
                "days_from_created",
                col("billing_record_total_rub").alias("current_1d_cons"),
                *prev_cons_columns
            )
            .join(dts, on=["billing_record_msk_date", "billing_account_id"], how="inner")
            .toPandas()
        )
        logger.info('Collected pandas.DataFrame')

        # general metrics
        total_cnt = df.shape[0]
        rmse = mean_squared_error(df['next_14d_cons'], df['next_14d_cons_pred'])**0.5
        mae = mean_absolute_error(df['next_14d_cons'], df['next_14d_cons_pred'])
        r2 = r2_score(df['next_14d_cons'], df['next_14d_cons_pred'])

        logger.info(f'General metrics for {last_ground_truth_date}')
        logger.info(f'Nobs: {total_cnt}')
        logger.info(f'RMSE: {rmse:.0f}')
        logger.info(f'MAE:  {mae:.1f}')
        logger.info(f'R2:   {r2:.3f}')

        spdf_gen_metrics = spark.createDataFrame(pd.DataFrame([{
            'rep_date': last_ground_truth_date,
            'rep_period_start': metrics_period_start,
            'rep_period_end': last_ground_truth_date,
            'total_cnt': total_cnt,
            'rmse': rmse,
            'mae': mae,
            'r2': r2,
        }]))

        date_grid = df['billing_record_msk_date'].unique()

        # onboarding metrics
        onb_calibr_by_dates = load_map_of_calibrators(yt_adapter, onb_calibr_path, date_grid)
        df_onb = df[(df[prev_cons_columns].sum(axis=1)==0) & (df['days_from_created']==14)].copy()
        df_onb = df_onb.reset_index(drop=True)

        df_onb['y_true'] = (df_onb['next_14d_cons']>=ONB_PARAMS['TARGET_PAID_COND']).astype(int)
        df_onb['y_proba'] = 0.0
        df_onb['y_pred'] = 0
        for idx in df_onb.index:
            obs_value = df_onb.loc[idx, 'next_14d_cons_pred']
            obs_date = df_onb.loc[idx, 'billing_record_msk_date']
            df_onb.loc[idx, 'y_proba'] = onb_calibr_by_dates[obs_date].predict([obs_value])[0]
            df_onb.loc[idx, 'y_pred'] = (df_onb.loc[idx, 'y_proba']>ONB_PARAMS['PRED_PROBA_BRD']).astype(int)

        onb_cnt = df_onb.shape[0]
        onb_gini_tgt = roc_auc_score(df_onb['y_true'], df_onb['y_pred'])*2-1
        onb_gini_rps = roc_auc_score(df_onb['y_true'], df_onb['y_proba'])*2-1
        onb_prec = precision_score(df_onb['y_true'], df_onb['y_pred'])
        onb_rec = recall_score(df_onb['y_true'], df_onb['y_pred'])
        onb_f1 = f1_score(df_onb['y_true'], df_onb['y_pred'])

        logger.info(f'Onboarding metrics for {last_ground_truth_date}')
        logger.info(f'Nobs:          {onb_cnt}')
        logger.info(f'Gini target:   {onb_gini_tgt:.3f}')
        logger.info(f'Gini response: {onb_gini_rps:.3f}')
        logger.info(f'Precision:     {onb_prec:.3f}')
        logger.info(f'Recall:        {onb_rec:.3f}')
        logger.info(f'F1:            {onb_f1:.3f}')

        spdf_onb_metrics = spark.createDataFrame(pd.DataFrame([{
            'rep_date': last_ground_truth_date,
            'rep_period_start': metrics_period_start,
            'rep_period_end': last_ground_truth_date,
            'total_cnt': total_cnt,
            'gini_tgt': onb_gini_tgt,
            'gini_rps': onb_gini_rps,
            'prec': onb_prec,
            'rec': onb_rec,
            'f1': onb_f1
        }]))

        # csm metrics
        csm_calibr_by_dates = load_map_of_calibrators(yt_adapter, csm_calibr_path, date_grid)
        df_csm = df[(df['prev_30d_cons']<CSM_PARAMS['TARGET_PAID_COND']) & (df['prev_30d_cons']>CSM_PARAMS['MIN_PAID_LAST_30D'])].copy()
        df_csm = df_csm.reset_index(drop=True)

        target_calc_cols = ['prev_15d_cons', 'current_1d_cons', 'next_14d_cons']
        df_csm['y_true'] = (df_csm[target_calc_cols].sum(axis=1)>=CSM_PARAMS['TARGET_PAID_COND']).astype(int)
        df_csm['y_proba'] = 0.0
        df_csm['y_pred'] = 0
        for idx in df_csm.index:
            obs_value = df_csm.loc[idx, ['prev_15d_cons', 'current_1d_cons', 'next_14d_cons_pred']].sum()
            obs_date = df_csm.loc[idx, 'billing_record_msk_date']
            df_csm.loc[idx, 'y_proba'] = csm_calibr_by_dates[obs_date].predict([obs_value])[0]
            df_csm.loc[idx, 'y_pred'] = (df_csm.loc[idx, 'y_proba']>CSM_PARAMS['PRED_PROBA_BRD']).astype(int)

        csm_cnt = df_csm.shape[0]
        csm_gini_tgt = roc_auc_score(df_csm['y_true'], df_csm['y_pred'])*2-1
        csm_gini_rps = roc_auc_score(df_csm['y_true'], df_csm['y_proba'])*2-1
        csm_prec = precision_score(df_csm['y_true'], df_csm['y_pred'])
        csm_rec = recall_score(df_csm['y_true'], df_csm['y_pred'])
        csm_f1 = f1_score(df_csm['y_true'], df_csm['y_pred'])

        logger.info(f'CSM metrics for {last_ground_truth_date}')
        logger.info(f'Nobs:           {csm_cnt}')
        logger.info(f'Gini target:    {csm_gini_tgt:.3f}')
        logger.info(f'Gini response:  {csm_gini_rps:.3f}')
        logger.info(f'Precision:      {csm_prec:.3f}')
        logger.info(f'Recall:         {csm_rec:.3f}')
        logger.info(f'F1:             {csm_f1:.3f}')

        spdf_csm_metrics = spark.createDataFrame(pd.DataFrame([{
            'rep_date': last_ground_truth_date,
            'rep_period_start': metrics_period_start,
            'rep_period_end': last_ground_truth_date,
            'total_cnt': total_cnt,
            'gini_tgt': csm_gini_tgt,
            'gini_rps': csm_gini_rps,
            'prec': csm_prec,
            'rec': csm_rec,
            'f1': csm_f1
        }]))

        logger.info('Saving report...')
        spdf_gen_metrics.write.yt(gen_metrics_path, mode='append')
        spdf_onb_metrics.write.yt(onb_metrics_path, mode='append')
        spdf_csm_metrics.write.yt(csm_metrics_path, mode='append')

    yt_adapter.optimize_chunk_number(gen_metrics_path)
    yt_adapter.optimize_chunk_number(onb_metrics_path)
    yt_adapter.optimize_chunk_number(csm_metrics_path)


if __name__ == '__main__':
    calc_prod_metrics()
