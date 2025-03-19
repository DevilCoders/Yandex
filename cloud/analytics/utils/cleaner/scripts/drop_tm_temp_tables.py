from clan_tools.utils.transfer_manager import drop_temp_tables
from clan_tools.secrets.Vault import Vault
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
import click
import json
from typing import List

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--table_to_clean', multiple=True)
def main(table_to_clean: str):
    logger.debug('Cleaning is started')
    tables_to_drop: List[str] = []
    Vault().get_secrets(add_to_env=True)
    for table in table_to_clean:
        tables_to_drop += drop_temp_tables(table)

    with open('output.json', 'w') as f:
        json.dump({"tables_to_drop": tables_to_drop}, f)


if __name__ == '__main__':
    main()
