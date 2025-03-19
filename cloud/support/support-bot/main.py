#!/usr/bin/env python3

import argparse
import logging

from logging.handlers import RotatingFileHandler

from utils.config import Config as cfg
from core.bot import TelegramBot
from core.version import __version__
from core.components import command_handlers, button_handlers
from core.workers import daily_workers, repeating_workers

parser = argparse.ArgumentParser(description='Yandex.Cloud Support Telegram Bot')
parser.add_argument('--version', action='version', version=__version__)
parser.add_argument('--proxy', action='store_true', help='connect to telegram API with proxy')
parser.add_argument('-l', '--write-logs', action='store_true', help='write logs to /var/log/ or path from config.')
args = parser.parse_args()

log_handlers = [logging.StreamHandler()]

if args.write_logs:
    log_handlers.append(
        RotatingFileHandler(
            cfg.LOGPATH,
            maxBytes=cfg.MAX_LOGFILE_SIZE,
            encoding='utf8',
            backupCount=cfg.LOG_BACKUP_COUNT
        )
    )

logging.basicConfig(
    level=cfg.LOGLEVEL,
    datefmt='%d/%b/%y %H:%M:%S',
    handlers=log_handlers,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

logger = logging.getLogger(__name__)


def get_proxy():
    """Detect proxy settings from config and return it."""
    _creds = [cred for cred in (cfg.PROXY_URL, cfg.PROXY_USER, cfg.PROXY_PASSWD) if cred is not None]
    specified = True if len(_creds) == 3 else False

    proxy_settings = {
        'read_timeout': cfg.READ_TIMEOUT or 60,
        'connect_timeout': cfg.CONNECT_TIMEOUT or 15,
        'proxy_url': cfg.PROXY_URL,
        'urllib3_proxy_kwargs': {
            'username': cfg.PROXY_USER,
            'password': cfg.PROXY_PASSWD
        }
    }

    if specified:
        return proxy_settings
    return


def main():
    """Init and run TelegramBot."""
    bot = TelegramBot(
        token=cfg.TELEGRAM_TOKEN,
        handlers=command_handlers,
        commands=button_handlers,
        repeating_workers=repeating_workers,
        daily_workers=daily_workers
    )
    bot.run(
        proxy=get_proxy() if args.proxy else {}
    )


if __name__ == '__main__':
    main()
