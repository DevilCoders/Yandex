"""
DBaaS E2E test
"""
import argparse
import logging
import logging.handlers
import sys
import traceback

from . import scenarios  # noqa
from .config import get_config
from .internal_api import ApiClient
from .utils import all_scenarios


def die(logger, status, scenario, error):
    """
    Prints to stdout status and error in monrun format
    """
    logger.error(str(error))
    logger.error(traceback.format_exc())
    print(
        '{status};{scenario}: {typ}: {err}'.format(
            scenario=scenario, status=status, typ=type(error).__name__, err=str(error).replace('\n', ' ')
        )
    )
    sys.exit(1)


def init_logging(config):
    """
    Initialize logging subsystem
    """
    root = logging.getLogger()
    root.setLevel(config.log_level)
    file_handler = logging.handlers.RotatingFileHandler(config.log_file, maxBytes=config.log_size)

    formatter = logging.Formatter(config.log_format)
    file_handler.setFormatter(formatter)

    root.addHandler(file_handler)

    if config.log_enable_stdout:
        stdout_handler = logging.StreamHandler(sys.stdout)
        stdout_handler.setFormatter(formatter)
        root.addHandler(stdout_handler)


def _main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('-c', '--config', type=str, default='/etc/dbaas-compute-e2e.json', help='Config file path')
    parser.add_argument(
        '-s', '--scenario', type=str, help='Select one scenario', choices=[x.__name__ for x in all_scenarios()]
    )

    args = parser.parse_args()
    config = get_config(args.config)
    init_logging(config)
    execute_scenarios(config, args.scenario)
    print('0;OK')


def execute_scenarios(config, selected_scenario):
    """
    Execute all registered scenarios
    """
    for scenario in all_scenarios():
        sname = scenario.__name__
        if selected_scenario is not None and sname != selected_scenario:
            continue
        logger = logging.getLogger(sname)
        try:
            logger.info('Starting scenario')
            api_client = getattr(scenario, 'API_CLIENT', '')
            get_api_client_kwargs = getattr(scenario, 'get_api_client_kwargs', lambda: {})
            api = ApiClient.from_type(api_client or config.api_client, config, logger, **get_api_client_kwargs())

            cluster_name = config.dbname + getattr(scenario, 'CLUSTER_NAME_SUFFIX', '')
            api.cleanup_folder(scenario.CLUSTER_TYPE, cluster_name)

            # Create new cluster and wait it
            operation = api.create_cluster(
                scenario.CLUSTER_TYPE,
                cluster_name,
                config.environment,
                scenario.get_options(config),
                wait=True,
            )

            # Execute post-checks
            hosts = api.cluster_hosts(scenario.CLUSTER_TYPE, operation.cluster_id)
            logger.info('post check for hosts %s', hosts)
            scenario.post_check(config, hosts, api_client=api, cluster_id=operation.cluster_id, logger=logger)
            api.cleanup_folder(scenario.CLUSTER_TYPE, cluster_name)
            logger.info('Ending scenario: Success')
        except Exception as error:
            logger.exception("get exception during scenario %s", sname)
            die(logger, 2, sname, error)


if __name__ == '__main__':
    _main()
