import json
import click
import logging.config
from datetime import datetime
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--models_path', default='//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/models_history')
def get_last_model_path(models_path: str):
    last_model_name = datetime.now().strftime('model_%Y%m%d_%H%M.bin')
    last_model_path = f'{models_path}/{last_model_name}'
    with open('output.json', 'w') as fp:
        json.dump({'path': last_model_path}, fp)


if __name__ == '__main__':
    get_last_model_path()
