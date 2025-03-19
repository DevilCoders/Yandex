
import luigi
import logging.config
from clan_tools.logging.logger import default_log_config
from cloud_antifraud.tasks.dashboard.TimeToBlock import TimeToBlock
import luigi.tools.deps_tree as deps_tree
import json
import logging.config



logging.config.dictConfig(default_log_config)
luigi.configuration.add_config_path('config/luigi.conf')


if __name__ == '__main__':
    task = TimeToBlock()
    print(deps_tree.print_tree(task))
    luigi.build([task], local_scheduler=True)

    with open('output.json', 'w') as f:
        json.dump({'status': 'success'}, f)

   