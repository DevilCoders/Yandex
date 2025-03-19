import logging.config
import os

import sys
import yaml

sys.path.append('../../transients_detection/src')
sys.path.append('../../telegram-bot-constructor/src')
sys.path.append('../src')

from clan_tools.utils.conf import read_conf

from clan_telegram_bot.bot.clan_time_allocation import get_bot

if __name__ == '__main__':
    # PROXY = "http://192.168.101.43:8080"
    # os.environ["http_proxy"] = PROXY
    # os.environ["https_proxy"] = PROXY
    # os.environ["HTTP_PROXY"] = PROXY
    # os.environ["HTTPS_PROXY"] = PROXY

    bot_conf = read_conf('config/clan_telegram_bot_dev_local.conf')
    logger_conf = read_conf('config/logger.conf')
    

    if not os.path.exists('logs'):
        os.makedirs('logs')

    logger_conf['handlers']['file_handler']['filename'] = 'logs/clan_telegram_bot.log'
    logging.config.dictConfig(logger_conf)

    cons = get_bot(bot_conf)
    cons.main()
