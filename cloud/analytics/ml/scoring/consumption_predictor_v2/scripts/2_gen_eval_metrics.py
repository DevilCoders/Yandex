import json
import click
import pandas as pd
from datetime import datetime
from os.path import join as path_join
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.timing import timing
from consumption_predictor_v2.utils.helpers_yt_wrapper import BR_MSK_DT_NAME, Y_TRUE_COL_NAME, Y_PRED_COL_NAME
from consumption_predictor_v2.utils.helpers_yt_wrapper import read_preprocess_catboost_result_yt

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def calculate_gen_metrics(df):
    df_metrics = []
    for dt in sorted(df[BR_MSK_DT_NAME].unique()):
        tdf = df[df[BR_MSK_DT_NAME]==dt].copy()
        metrics = {}
        metrics.update({'RMSE': mean_squared_error(tdf[Y_TRUE_COL_NAME], tdf[Y_PRED_COL_NAME])**0.5})
        metrics.update({'MAE': mean_absolute_error(tdf[Y_TRUE_COL_NAME], tdf[Y_PRED_COL_NAME])})
        metrics.update({'R2': r2_score(tdf[Y_TRUE_COL_NAME], tdf[Y_PRED_COL_NAME])})
        df_metrics.append(metrics)
    df_metrics = pd.DataFrame(df_metrics)

    res = {}
    res.update({f'{key}_mean': val for key, val in dict(df_metrics.mean()).items()})
    res.update({f'{key}_std': val for key, val in dict(df_metrics.std()).items()})
    res.update({f'{key}_ci_lower': val for key, val in dict(df_metrics.quantile(q=0.025)).items()})
    res.update({f'{key}_ci_upper': val for key, val in dict(df_metrics.quantile(q=0.975)).items()})
    return res


@click.command()
@click.option('--metrics_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics/train_gen_metrics")
@click.option('--samples_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_train_samples")
@click.option('--models_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/models_history")
def eval_metrics_prod(metrics_path: str, samples_path: str, models_path: str):
    yt_adapter = YTAdapter()

    logger.info('Loading datasets...')
    last_model_name = max(yt_adapter.yt.list(models_path))
    df_train_val = read_preprocess_catboost_result_yt(yt_adapter, path_join(samples_path, 'result_train_val'))
    df_test = read_preprocess_catboost_result_yt(yt_adapter, path_join(samples_path, 'result_test'))
    df_oot = read_preprocess_catboost_result_yt(yt_adapter, path_join(samples_path, 'result_oot'))

    logger.info('Calculating metrics...')
    res_df = []
    for dataset, df in [('train_val', df_train_val), ('test', df_test), ('oot', df_oot)]:
        row = {
            'updated': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'model_name': 'Cons_predictor-train-gen',
            'model_version': last_model_name,
            'dataset_name': dataset
        }
        key_metrics = calculate_gen_metrics(df)
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
    eval_metrics_prod()
