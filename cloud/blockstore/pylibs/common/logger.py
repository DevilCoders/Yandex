import logging

_DEFAULT_FORMAT = '[ %(name)-8s ] %(asctime)s [ %(levelname)-8s ] %(message)s'
_TEAMCITY_FORMAT = '##teamcity[message text=\'|[ %(name)-8s |] %(message)s\' status=\'NORMAL\' ]'


def create_logger(name: str, args):
    """Creates logger instance with custom handlers.

    Use args.quite/args.verbose to set verbosity level.
    Use args.teamcity to set teamcity logging format.

    Attributes:
      name: logger name.
      args: instance returned by argparse parse_args() function.
    """
    logger = logging.getLogger(name)

    log_level = logging.INFO
    if getattr(args, 'quite', False):
        log_level = logging.CRITICAL
    elif getattr(args, 'verbose', False):
        log_level = logging.DEBUG

    logger.setLevel(log_level)
    handler = logging.StreamHandler()
    handler.setLevel(log_level)

    logger_format = _DEFAULT_FORMAT
    if getattr(args, 'teamcity', False):
        logger_format = _TEAMCITY_FORMAT

    formatter = logging.Formatter(logger_format)
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    return logger
