import json
import click
import logging.config
import pandas as pd
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--rebuild', is_flag=True, default=False)
def main(rebuild: bool = False) -> None:

    month_list = pd.date_range('2021-01-01', 'today', freq='MS').strftime("%Y-%m").tolist()
    if not rebuild:
        month_list = sorted(month_list)[-2:]
    logger.debug(month_list)
    with open('output.json', 'w') as f:
        json.dump({"$dates" : month_list}, f)

if __name__ == "__main__":
    main()
