import json
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--metrics_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/metrics/train_csm_metrics")
def get_last_csm_metrics(metrics_path: str):
    yt_adapter = YTAdapter()

    logger.info('Loading last CSM metrics')
    df_metrics = yt_adapter.read_table(metrics_path)
    df_metrics = df_metrics[df_metrics['dataset_name']=='oot']
    df_metrics = df_metrics[df_metrics['model_name']=='Cons_predictor-train-csm']
    df_metrics = df_metrics[df_metrics['updated']==df_metrics['updated'].max()].reset_index(drop=True)
    df_metrics = df_metrics.drop(columns=['dataset_name', 'model_name', 'updated']).T[0]
    metrics = dict(df_metrics)

    with open('output.json', 'w') as fp:
        json.dump(metrics, fp)


if __name__ == '__main__':
    get_last_csm_metrics()
