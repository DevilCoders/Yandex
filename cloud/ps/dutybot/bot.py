#!/usr/bin/env python3

import sys

from bot_utils.config import Config
from bot_utils.environment import get_messenger_and_environment, set_logger
from bots.telegram import TelegramDutyBot
from bots.yandex import YandexMessengerDutyBot


if __name__ == "__main__":
    messenger, env = get_messenger_and_environment(sys.argv)
    set_logger(env)
    if messenger == "telegram":
        TelegramDutyBot(Config(), env).run()
    elif messenger == "yandex":
        YandexMessengerDutyBot(Config(), env).run()
    else:
        raise ValueError(f"Not supported messenger: {messenger}")
