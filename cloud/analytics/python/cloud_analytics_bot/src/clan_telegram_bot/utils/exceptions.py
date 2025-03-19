import logging
import traceback


class BotException(Exception):
    pass


class BotDbException(BotException):
    pass


logger = logging.getLogger(__name__)

