import json
import sys
import subprocess  # noqa
from typing import List
import logging
import logging.handlers
import marshmallow
from dataclasses import dataclass, field

import click
import marshmallow_dataclass
from ylock import create_manager


logger = logging.getLogger(__name__)


@dataclass
class LoggingConfig:
    path: str = field(metadata=dict(required=True))
    level: str = field(default='INFO')
    zklevel: str = field(default='INFO')


@dataclass
class Config:
    host: List[str] = field(metadata=dict(required=True))
    logger: LoggingConfig = field(metadata=dict(required=True))
    app_id: str = field(metadata=dict(required=True))
    timeout: int = field(default=5)


def load_config(filename: str) -> Config:
    with open(filename) as fd:
        raw_config = json.load(fd)
        schema = marshmallow_dataclass.class_schema(Config)()
        return schema.load(raw_config)


def init_logging(config: LoggingConfig) -> None:
    handler = logging.handlers.WatchedFileHandler(filename=config.path)
    handler.setFormatter(
        logging.Formatter('%(asctime)s %(name)s %(levelname)-8s [%(process)d]: %(message)s'),
    )
    for zk_logger_name in ['ylock', 'kazoo']:
        zk_logger = logging.getLogger(zk_logger_name)
        zk_logger.setLevel(logging.getLevelName(config.zklevel))
        zk_logger.addHandler(handler)
    logger.addHandler(handler)
    logger.setLevel(logging.getLevelName(config.level))


def lock_and_run(lock: str, cmd: str, cfg: Config, cannot_lock_exitcode: int) -> int:
    manager = create_manager('zookeeper', hosts=cfg.host, prefix=cfg.app_id, connect_timeout=cfg.timeout)
    with manager.lock(lock) as locked:
        if not locked:
            logger.info("lock not acquired")
            return cannot_lock_exitcode

        logger.info("lock acquired")
        return subprocess.call(cmd, shell=True)  # noqa


@click.command()
@click.argument('lock')
@click.argument('cmd')
@click.option('--config', '-c', type=click.Path(exists=True), required=True)
@click.option('--exitcode', '-x', type=int, default=0, show_default=True, help='Exit code if lock isn\'t acquired.')
def main(lock: str, cmd: str, config: str, exitcode: int) -> None:
    try:
        cfg = load_config(config)
    except (ValueError, marshmallow.exceptions.ValidationError) as exc:
        print("malformed config: %s" % exc)
        sys.exit(1)

    init_logging(cfg.logger)
    sys.exit(lock_and_run(lock, cmd, cfg, exitcode))
