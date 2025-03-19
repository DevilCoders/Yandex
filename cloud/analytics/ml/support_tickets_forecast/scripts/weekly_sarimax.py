import os
import logging.config
from datetime import datetime, timedelta

import click
import numpy as np
import pandas as pd
from scipy.stats import norm
import statsmodels.api as sm
import pyspark.sql.functions as F

from spyt import spark_session
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.utils.spark import safe_append_spark
from clan_tools.utils.time import DATE_FORMAT
from clan_tools.utils.time import parse_date
from support_tickets_forecast.data import TicketsDataset
from support_tickets_forecast.data import linear_fix_ts
from support_tickets_forecast.time_features import safe_resample_weekly
from support_tickets_forecast.time_features import get_date_sin_cos_features_weekly
from support_tickets_forecast.time_features import DAYS_OF_WEEK_REV


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

SM_PRED_COLS = ['mean', 'mean_se', 'mean_ci_lower', 'mean_ci_upper']


def forecast_to_date(y: pd.Series, dt: str, ci: float = 0.7) -> pd.DataFrame:
    y = y[y.index <= parse_date(dt)].copy()

    assert 0.0 <= ci <= 1.0, f'Confidence interval must be between 0.0 and 1.0, but it is {ci}'
    n_std = (norm.ppf(1-(1-ci)/2)-norm.ppf((1-ci)/2))/2

    dt_from, dt_to = y.index.min().strftime(DATE_FORMAT), y.index.max().strftime(DATE_FORMAT)
    sampling = 'W-'+DAYS_OF_WEEK_REV[y.index.max().weekday()]
    y_weekly = safe_resample_weekly(y, sampling)

    y_weekly = linear_fix_ts(y_weekly, '2022-03-10', '2022-05-01')
    exog_current = get_date_sin_cos_features_weekly(dt_from, dt_to, 0)
    future = y.index.max() + timedelta(weeks=52)
    exog_future = get_date_sin_cos_features_weekly(dt_to, future.strftime(DATE_FORMAT), 0)

    try:
        model = sm.tsa.SARIMAX(y_weekly, exog=exog_current, order=(0, 1, 0), trend='ct').fit(maxiter=300, disp=0)
        prediction = model.get_forecast(future, exog=exog_future).summary_frame()
        prediction['mean_ci_lower'] = np.maximum(prediction['mean_ci_lower'], 0)
        prediction.index = prediction.index.strftime(DATE_FORMAT)
    except Exception:
        prediction = pd.DataFrame(columns=SM_PRED_COLS)
        prediction[SM_PRED_COLS] = prediction[SM_PRED_COLS].astype(float)
    prediction['train_date'] = dt
    prediction['mean_ci_lower'] = prediction['mean'] - n_std * prediction['mean_se']
    prediction['mean_ci_upper'] = prediction['mean'] + n_std * prediction['mean_se']
    prediction.index.name = 'rep_date'
    prediction = prediction.reset_index()

    return prediction


@click.command()
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def make_forecast(is_prod: bool = False, rebuild: bool = False) -> None:
    logger.info('Starting driver...')

    rebuild = rebuild or not is_prod
    postfix = '' if is_prod else '_test'
    results_yt_path = '//home/cloud_analytics/ml/support_tickets_forecast/weekly' + postfix

    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        logger.info('Collecting data...')
        ticket_dataset = TicketsDataset(spark, yt_adapter)
        df_target = ticket_dataset.get_y()
        df_target = df_target[df_target['components_quotas'] == False]  # noqa: E712
        df_target = df_target['count'].resample('1D').sum().fillna(0)

        logger.info('Collecting info...')
        date_yesterday = (datetime.now() - timedelta(days=1)).strftime(DATE_FORMAT)
        data_last_available = df_target.index.max().strftime(DATE_FORMAT)
        date_to = min(date_yesterday, data_last_available)
        if yt_adapter.yt.exists(results_yt_path) and not rebuild:
            last_train_date = spark.read.yt(results_yt_path).agg(F.max('train_date')).collect()[0][0]
            date_from = (parse_date(last_train_date) + timedelta(days=1)).strftime(DATE_FORMAT)
        else:
            if rebuild and yt_adapter.yt.exists(results_yt_path):
                yt_adapter.yt.remove(results_yt_path)
            if is_prod:
                date_from = '2021-01-01'
            else:
                date_from = (datetime.now() - timedelta(days=3)).strftime(DATE_FORMAT)

        logger.info(f'Calculating from {date_from} to {date_to}...')
        res = pd.DataFrame()
        for dt in pd.date_range(date_from, date_to).strftime(DATE_FORMAT):
            pred = forecast_to_date(df_target, dt)
            res = pd.concat([res, pred], axis=0, ignore_index=True)

        logger.info('Saving results...')
        spdf_res = spark.createDataFrame(res)
        safe_append_spark(yt_adapter, spdf_res, results_yt_path)

    yt_adapter.optimize_chunk_number(results_yt_path)


if __name__ == '__main__':
    make_forecast()
