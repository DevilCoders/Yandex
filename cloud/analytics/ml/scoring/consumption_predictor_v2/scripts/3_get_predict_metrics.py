import json
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--metrics_path')
@click.option('--date_column', default='rep_date')
def get_last_metrics_by_column(metrics_path: str, date_column: str):
    yt_adapter = YTAdapter()

    logger.info('Loading metrics table')
    df_metrics = yt_adapter.read_table(metrics_path)
    max_rep_date = df_metrics[date_column].max()
    df_metrics = df_metrics[df_metrics[date_column]==max_rep_date].reset_index(drop=True)
    df_metrics = df_metrics.drop(columns=[date_column, 'rep_period_start', 'rep_period_end']).T[0]
    metrics = dict(df_metrics)

    with open('output.json', 'w') as fp:
        json.dump(metrics, fp)


if __name__ == '__main__':
    get_last_metrics_by_column()
