import json
import click
import pickle
import pandas as pd
from datetime import datetime
from os.path import join as path_join
from sklearn.metrics import roc_auc_score, precision_score, recall_score, f1_score
from sklearn.isotonic import IsotonicRegression
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.utils.timing import timing
from consumption_predictor_v2.utils.helpers_yt_wrapper import BA_ID_COL_NAME, BR_MSK_DT_NAME, Y_TRUE_COL_NAME, Y_PRED_COL_NAME
from consumption_predictor_v2.utils.helpers_yt_wrapper import read_preprocess_catboost_result_yt
from consumption_predictor_v2.train_config.onb_params import PARAMS

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

ONB_TARGET_COL_NAME = 'onb_target'
ONB_RESPONSE_COL_NAME = 'onb_proba'


@timing
def calculate_onb_metrics(df, n_period=10):
    brd = PARAMS['PRED_PROBA_BRD']
    p_val = 1-PARAMS['CONF_INTERVAL']
    df_metrics = []
    all_dates = sorted(df[BR_MSK_DT_NAME].unique())
    chunked_dates = [all_dates[i:i+n_period] for i in range(0, len(all_dates), n_period)]
    for dt_chunk in chunked_dates:
        tdf = df[df[BR_MSK_DT_NAME].isin(dt_chunk)].copy()
        if tdf[ONB_TARGET_COL_NAME].nunique()==2:
            metrics = {}
            metrics.update({'Gini_rsp': roc_auc_score(tdf[ONB_TARGET_COL_NAME], tdf[ONB_RESPONSE_COL_NAME])*2-1})
            metrics.update({'Gini_tgt': roc_auc_score(tdf[ONB_TARGET_COL_NAME], tdf[ONB_RESPONSE_COL_NAME]>brd)*2-1})
            metrics.update({'Precision': precision_score(tdf[ONB_TARGET_COL_NAME], tdf[ONB_RESPONSE_COL_NAME]>brd)})
            metrics.update({'Recall': recall_score(tdf[ONB_TARGET_COL_NAME], tdf[ONB_RESPONSE_COL_NAME]>brd)})
            metrics.update({'F1_Score': f1_score(tdf[ONB_TARGET_COL_NAME], tdf[ONB_RESPONSE_COL_NAME]>brd)})
            df_metrics.append(metrics)
    df_metrics = pd.DataFrame(df_metrics)

    res = {}
    res.update({f'{key}_mean': val for key, val in dict(df_metrics.mean()).items()})
    res.update({f'{key}_std': val for key, val in dict(df_metrics.std()).items()})
    res.update({f'{key}_ci_lower': val for key, val in dict(df_metrics.quantile(q=p_val/2)).items()})
    res.update({f'{key}_ci_upper': val for key, val in dict(df_metrics.quantile(q=1-p_val/2)).items()})
    return res


@click.command()
@click.option('--metrics_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics/train_onb_metrics")
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--samples_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_train_samples")
@click.option('--models_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/models_history")
@click.option('--calibrators_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/calibrators_history/onboarding")
def onb_eval_metrics_prod(metrics_path: str, features_path: str, samples_path: str, models_path: str, calibrators_path: str):
    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()

    logger.info('Loading and preparing datasets...')
    last_model_name = max(yt_adapter.yt.list(models_path))
    last_model_dt = datetime.strptime(last_model_name, 'model_%Y%m%d_%H%M.bin')
    df_train_val = read_preprocess_catboost_result_yt(yt_adapter, path_join(samples_path, 'result_train_val'))
    df_test = read_preprocess_catboost_result_yt(yt_adapter, path_join(samples_path, 'result_test'))
    df_oot = read_preprocess_catboost_result_yt(yt_adapter, path_join(samples_path, 'result_oot'))

    paid_cond = PARAMS['TARGET_PAID_COND']
    df_train_val[ONB_TARGET_COL_NAME] = (df_train_val[Y_TRUE_COL_NAME]>=paid_cond).astype(int)
    df_test[ONB_TARGET_COL_NAME] = (df_test[Y_TRUE_COL_NAME]>=paid_cond).astype(int)
    df_oot[ONB_TARGET_COL_NAME] = (df_oot[Y_TRUE_COL_NAME]>=paid_cond).astype(int)

    selecting_cols = '`'+'`, `'.join([BA_ID_COL_NAME, BR_MSK_DT_NAME, 'days_from_created'])+'`'
    min_date = df_train_val[BR_MSK_DT_NAME].min()
    query = f"""SELECT {selecting_cols} FROM `{features_path}` WHERE `billing_record_msk_date` >= '{min_date}' AND `days_from_created` = 14"""
    df_features = yql_adapter.run_query_to_pandas(query)

    tr_df_train_val = df_train_val.merge(df_features, on=[BA_ID_COL_NAME, BR_MSK_DT_NAME], how='inner')
    tr_df_test = df_test.merge(df_features, on=[BA_ID_COL_NAME, BR_MSK_DT_NAME], how='inner')
    tr_df_oot = df_oot.merge(df_features, on=[BA_ID_COL_NAME, BR_MSK_DT_NAME], how='inner')

    logger.info('Calibration fitting and saving calibrator...')
    calibr_onb = IsotonicRegression(out_of_bounds='clip', y_min=PARAMS['MIN_CALIBR_PROBA'], y_max=PARAMS['MAX_CALIBR_PROBA'])
    calibr_onb.fit(df_train_val[Y_PRED_COL_NAME], df_train_val[ONB_TARGET_COL_NAME])

    tr_df_train_val[ONB_RESPONSE_COL_NAME] = calibr_onb.predict(tr_df_train_val[Y_PRED_COL_NAME])
    tr_df_test[ONB_RESPONSE_COL_NAME] = calibr_onb.predict(tr_df_test[Y_PRED_COL_NAME])
    tr_df_oot[ONB_RESPONSE_COL_NAME] = calibr_onb.predict(tr_df_oot[Y_PRED_COL_NAME])

    calibr_onb_ser = pickle.dumps(calibr_onb)
    corr_calibr_name = last_model_dt.strftime('onb_calibr_%Y%m%d_%H%M.pkl')
    yt_adapter.yt.write_file(f'{calibrators_path}/{corr_calibr_name}', calibr_onb_ser)

    logger.info('Caclulating Onboarding metrics...')
    res_df = []
    for dataset, df in [('train_val', tr_df_train_val), ('test', tr_df_test), ('oot', tr_df_oot)]:
        row = {
            'updated': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'model_name': 'Cons_predictor-train-onb',
            'dataset_name': dataset
        }
        key_metrics = calculate_onb_metrics(df)
        row.update(key_metrics)
        res_df.append(row)
    res_df = pd.DataFrame(res_df)

    logger.info('Saving metrics...')
    yt_schema = yt_adapter.get_pandas_default_schema(res_df)
    yt_adapter.save_result(metrics_path, yt_schema, res_df)
    yt_adapter.optimize_chunk_number(metrics_path)

    with open('output.json', 'w') as fp:
        json.dump(key_metrics, fp)


if __name__ == '__main__':
    onb_eval_metrics_prod()
