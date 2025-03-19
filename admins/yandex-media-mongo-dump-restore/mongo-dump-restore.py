import argparse
import logging
import os
import yaml
from dotmap import DotMap
from dump_restore import Worker

LOG = logging.getLogger('Global')


def load_config(filename='/etc/yandex/yandex-media-mongo-dump-restore/main.yml'):

    def load(loader, node):
        path = os.path.dirname(filename) + '/' + node.value
        LOG.debug("include config %s", path)
        data = yaml.load(open(path))
        return data

    yaml.add_constructor('!include', load)

    LOG.info("Open config %s", filename)
    # filename = '/etc/yandex/yandex-tv-mongo-refiller/main.yml'
    if not os.path.exists(filename):
        filename = '/etc/yandex/yandex-tv-mongo-refiller/main-default.yml'
        LOG.info("Config not found in {}".format(filename))
    if not os.path.exists(filename):
        LOG.error("Config not found in {}".format(filename))
        return None

    with open(filename) as f:
        conf = f.read()

    config = yaml.load(conf)
    return DotMap(config)


def parse_args():
    """
    parse arguments
    :return: args
    """
    parser = argparse.ArgumentParser(description="Backup and restore tool for mysql/mongo using s3")
    parser.add_argument('-n', '--names', help='Config names to run(comma-separated list)',
                        action='store', default=None)
    return parser.parse_args()


def set_log_level(lvl):
    if lvl == 'DEBUG':
        return logging.DEBUG
    elif lvl == 'WARN':
        return logging.WARN
    elif lvl == 'ERROR':
        return logging.ERROR
    elif lvl == 'INFO':
        return logging.INFO
    else:
        return logging.INFO


def main():

    # config = load_config(filename='/home/chrono/dev/tokk-mongo-backup/conf/main.yml')
    config = load_config()
    args = parse_args()
    names = list(args.names.split(',')) if args.names else None

    lvl = set_log_level(config.logger.level)
    if config.logger.file:
        logging.basicConfig(level=lvl,
                            format='%(asctime)s %(name)10s %(levelname)s: %(message)s',
                            filename=config.logger.file)
    else:
        logging.basicConfig(level=lvl,
                            format='%(asctime)s %(name)10s %(levelname)s: %(message)s')

    log = logging.getLogger('Main')
    log.info("Partial run. Working with specified configs: %s", names)
    worker = Worker(config.configs, run_parts=names) if names else Worker(config.configs)
    worker.execute()


main()
