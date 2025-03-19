import os
import logging.config
import pandas as pd
import numpy as np
import statsmodels.api as sm
from datetime import timedelta, datetime
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import DEFAULT_SPARK_CONF

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def predict_fip4_ts(df, cfg):
    # define params
    p_val = 0.01
    state = df['billing_account_state'].dropna().iloc[0]
    status = df['billing_account_usage_status'].dropna().iloc[0]
    segment = df['crm_segment'].dropna().iloc[0]
    calc_date = df['calc_date'].dropna().iloc[0]
    predict_params = cfg[segment][state][status]
    f_days = predict_params['forecast_days']
    h_days = predict_params['history_days']
    if predict_params['date_from'] == '':
        dt_from = df['rep_date'].min()
    else:
        dt_from = predict_params['date_from']

    # make normal timeseries
    ts_df = df[df['rep_date']>=dt_from].sort_values(by='rep_date')
    new_index = pd.to_datetime(ts_df['rep_date'])
    ts_df = ts_df.set_index(new_index.values)
    ts = ts_df['ip_count'].iloc[-h_days:]

    # prediction date borders
    dt_fmt = '%Y-%m-%d'
    from_date = datetime.strptime(df['rep_date'].max(), dt_fmt) + timedelta(days=1)
    to_date = datetime.strptime(df['rep_date'].max(), dt_fmt) + timedelta(days=f_days)

    # predict const graph
    if predict_params['forecast_type'] == 'const':
        ts = ts.resample('D').max().fillna(0)
        mean = int(round(ts.median()))
        mean_se = round(ts.std(), 2)
        mean_ci_lower = max(int(round(ts.quantile(q=p_val*0.5)-1)), 0)
        mean_ci_upper = int(round(ts.quantile(q=1-p_val*0.5)+1))

        df_pred = pd.DataFrame(index=pd.date_range(from_date, to_date))
        df_pred['mean'] = mean
        df_pred['mean_se'] = mean_se
        df_pred['mean_ci_lower'] = mean_ci_lower
        df_pred['mean_ci_upper'] = mean_ci_upper
        result_df = pd.concat([ts.rename('ip_count'), df_pred], axis=1)

    # forward fill ts for stasmodels library
    ts = ts.resample('D').max().ffill()

    # predict simple graph
    if predict_params['forecast_type'] == 'simple':
        simple_model = sm.tsa.SARIMAX(ts, order=(1, 1, 1), trend='t').fit(maxiter=200)
        try:
            df_pred = simple_model.get_forecast(to_date).summary_frame().clip(lower=0)
            df_pred['rep_date'] = df_pred.index.strftime(dt_fmt)
            result_df = pd.concat([ts.rename('ip_count'), df_pred], axis=1)
        except Exception:
            result_df = pd.DataFrame(ts.rename('ip_count'))
            result_df['mean'] = np.nan
            result_df['mean_se'] = np.nan
            result_df['mean_ci_lower'] = np.nan
            result_df['mean_ci_upper'] = np.nan
        result_df['rep_date'] = result_df.index.strftime(dt_fmt)

    # predict advanced graph with params
    if predict_params['forecast_type'] == 'advanced':
        adv_params = predict_params['params']
        par_tr = adv_params['trend']
        par_p = adv_params['p']
        par_d = adv_params['d']
        par_q = adv_params['q']
        par_P = adv_params['P']
        par_D = adv_params['D']
        par_Q = adv_params['Q']
        par_S = adv_params['S']

        adv_model = sm.tsa.SARIMAX(
            ts,
            order=(par_p, par_d, par_q),
            seasonal_order=(par_P, par_D, par_Q, par_S),
            trend=par_tr).fit(maxiter=200)
        try:
            df_pred = adv_model.get_forecast(to_date).summary_frame().clip(lower=0)
            df_pred['rep_date'] = df_pred.index.strftime(dt_fmt)
            result_df = pd.concat([ts.rename('ip_count'), df_pred], axis=1)
        except Exception as exc:
            print(exc)
            result_df = pd.DataFrame(ts.rename('ip_count'))
            result_df['mean'] = np.nan
            result_df['mean_se'] = np.nan
            result_df['mean_ci_lower'] = np.nan
            result_df['mean_ci_upper'] = np.nan
        result_df['rep_date'] = result_df.index.strftime(dt_fmt)

    # final processing
    result_df['rep_date'] = result_df.index.strftime(dt_fmt)
    result_df = result_df.reset_index(drop=True)
    result_df['calc_date'] = calc_date
    result_df['billing_account_state'] = state
    result_df['billing_account_usage_status'] = status
    result_df['crm_segment'] = segment

    return result_df


def make_forecast_fip4_consumtion(source_path: str,
                                  result_table_path: str,
                                  config: dict,
                                  spark_conf: dict = DEFAULT_SPARK_CONF,
                                  chunk: int = 200*1024**2):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yta = YTAdapter(yt_token).yt
    #  spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=spark_conf) as spark:
        spyt.info(spark)

        calc_date = spark.read.yt(source_path).select('calc_date').agg(F.max('calc_date')).collect()[0][0]
        spdf = spark.read.yt(source_path).filter(col('calc_date')==calc_date)

        result_table_name = result_table_path.split('/')[-1]
        result_table_folder = '/'.join(result_table_path.split('/')[:-1])
        if result_table_name in yta.list(result_table_folder):
            from_dt = spark.read.yt(result_table_path).agg(F.max("calc_date")).collect()[0][0]
            from_dt = (datetime.strptime(from_dt, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
        else:
            from_dt = spark.read.yt(source_path).agg(F.min("calc_date")).collect()[0][0]

        to_dt = spark.read.yt(source_path).agg(F.max("calc_date")).collect()[0][0]
        dates = pd.date_range(start=from_dt, end=to_dt, freq='D').strftime('%Y-%m-%d')

        # calculation
        schema_list = [
            'ip_count double',
            'mean double',
            'mean_se double',
            'mean_ci_lower double',
            'mean_ci_upper double',
            'rep_date string',
            'calc_date string',
            'billing_account_state string',
            'billing_account_usage_status string',
            'crm_segment string',
        ]

        for i, date in enumerate(dates):
            print('Iter. %3d: Prediction for date %s' % (i+1, date))
            dff = (
                spdf.filter(col('calc_date')==date)
                .groupby(["billing_account_state", "billing_account_usage_status", "crm_segment"])
                .applyInPandas(lambda x: predict_fip4_ts(df=x, cfg=config), schema=', '.join(schema_list))
            )
            dff.write.optimize_for("scan").yt(result_table_path, mode='append')
    logger.info('Result table optimization')
    yta.transform(result_table_path, desired_chunk_size=chunk, optimize_for="scan")
