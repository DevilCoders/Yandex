#!/usr/bin/env python3
"""This module contains helper functions."""

import re
import os
import sys
import time
import yaml
import logging
import platform

from decorator import decorate
from prettytable import PrettyTable
from functools import wraps

from app.quota.error import ConvertValueError, ValidateError

logger = logging.getLogger(__name__)




class Color:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGNETA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    GREY = '\033[90m'
    END = '\033[0m'

    @staticmethod
    def make(target: [str, int, float, list], color='white'):
        """Colorize text and list objects.

        int, float will be converted to str.

        Available text colors:
          red, green, yellow, blue, magenta, cyan, white, grey.

        """
        if color.upper() not in dir(Color):
            raise ValueError('Unknown color received.')

        if target is None or not target:
            return target

        color = getattr(Color, color.upper())
        end = getattr(Color, 'END')

        if isinstance(target, list):
            colored_list = list()
            for obj in target:
                colored_list.append(f'{color}{obj}{end}')
            return colored_list

        return f'{color}{target}{end}'


def bytes_to_human(data: int, granularity=2):
    """Convert bytes to human format with binary prefix."""
    _bytes = int(data)
    result = []
    sizes = (
        ('TB', 1024 ** 4),
        ('GB', 1024 ** 3),
        ('MB', 1024 ** 2),
        ('KB', 1024),
        ('B', 1)
    )
    if _bytes == 0:
        return 0
    else:
        for name, count in sizes:
            value = _bytes // count
            if value:
                _bytes -= value * count
                result.append(f'{value} {name}')
        return ', '.join(result[:granularity])


def human_to_bytes(size: str):
    """Convert human size to bytes. Like a 1G -> 1073741824.

    Available sizes:
      B - byte
      K - kibibyte
      M - mebibyte
      G - gibibyte
      T - tebibyte

    """
    if re.fullmatch(r'^[\d{1,20}]+[Bb]$', size):
        new_size = size[:-1]
        return int(new_size)
    elif re.fullmatch(r'^[\d{1,20}]+[Kk]$', size):
        separator = size[:-1]
        new_size = int(separator) * 1024
        return new_size
    elif re.fullmatch(r'^[\d{1,20}]+[Mm]$', size):
        separator = size[:-1]
        new_size = int(separator) * 1024 ** 2
        return new_size
    elif re.fullmatch(r'^[\d{1,20}]+[Gg]$', size):
        separator = size[:-1]
        new_size = int(separator) * 1024 ** 3
        return new_size
    elif re.fullmatch(r'^[\d{1,20}]+[Tt]$', size):
        separator = size[:-1]
        new_size = int(separator) * 1024 ** 4
        return new_size
    else:
        raise ConvertValueError('Invalid size. Please, type correct size, examples: 1B, 10K, 100M, 100G, 2T')


def warning_limit_threshold(limit: int, usage: int, threshold=10):
    """If the percent difference between two numbers is greater than
    or equal to the threshold, returns True.

    """
    try:
        assert limit != 0
        assert usage != 0
    except AssertionError:
        return False

    diff = ((limit - usage) / limit) * 100  # percent diff
    if abs(int(diff)) <= int(threshold) and abs(int(diff)) != 0:
        return True
    return False


def print_as_table(data: [list, object], fields=None):
    """Colorize and print pretty table."""
    table = PrettyTable()
    table.field_names = fields or ['Name', 'Limit', 'Usage']
    table.align = 'l'

    data = data if isinstance(data, list) else data.metrics

    for value in data:
        if warning_limit_threshold(value.limit, value.usage):
            rows = Color.make([value.name, value.human_size_limit, value.human_size_usage], color='yellow')
        elif value.usage >= value.limit:
            rows = Color.make([value.name, value.human_size_limit, value.human_size_usage], color='red')
        else:
            rows = [value.name, value.human_size_limit, value.human_size_usage]

        table.add_row(rows)

    print(table)


def log(func, *args, **kwargs):
    """Add debug messages to logger."""
    logger = logging.getLogger(func.__module__)

    def decorator(self, *args, **kwargs):
        logger.debug(f'Entering {func.__name__}')
        result = func(*args, **kwargs)
        logger.debug(vars())
        logger.debug(result)
        logger.debug(f'Exiting {func.__name__}')
        return result

    return decorate(func, decorator)


def debug_msg():
    _info = (
        sys.version.split('\n'), platform.version(),
        f'System: {platform.system()}', f'Machine: {platform.machine()}',
        f'Platform: {platform.platform()}', platform.uname(),
    )

    for msg in _info:
        logger.debug(msg)


@log
def retry(exceptions, tries=4, delay=5, backoff=2, log=True):
    def retry_decorator(func):
        @wraps(func)
        def func_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return func(*args, **kwargs)
                except exceptions:
                    msg = f'Connection error. Retrying in {mdelay} seconds...'
                    logger.warning(msg) if log else print(msg)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return func(*args, **kwargs)
        return func_retry
    return retry_decorator


def init_config(config_dir, config_file):
    """Generate config file."""
    if not os.path.exists(config_dir):
        print('Creating config directory...')
        os.system(f'mkdir -p {config_dir}')

    print('Get OAuth-token here: ' +
          'https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb')
    token = input('Enter your OAuth-token: ').strip()
    billing_metadata = input('Enable displaying the billing account balance by default? [y/n]: ')

    if not os.path.isfile(os.path.join(config_dir, 'allCAs.pem')):
        print('\nTrying download allCAs.pem...')
        os.system(f'wget https://crls.yandex.net/allCAs.pem -q --show-progress --tries=3 -O {config_dir}/allCAs.pem')
        ssl_path = os.path.join(config_dir, 'allCAs.pem')
        if not os.path.isfile(os.path.join(config_dir, 'allCAs.pem')):
            print('\nPlease download CA cert from here: https://crls.yandex.net/allCAs.pem')
            ssl_path = input('And enter absolute path, example: /path/to/allCAs.pem: ').strip()
    else:
        ssl_path = os.path.join(config_dir, 'allCAs.pem')

    with open(config_file, 'w') as cfg:
        config = {
            'token': token,
            'ssl': ssl_path,
            'billing_metadata': True if billing_metadata.strip().lower() in ('y', 'yes') else False
        }

        cfg.write(yaml.dump(config, sort_keys=False))

    cfg.close()


def interactive_help():
    messages = [
        '\nPress ENTER to make no changes for metric',
        'Press Ctrl+C or type "q" to abort updating and print metrics',
        'All sizes must be B, K, M, G, T (example: 1B, 10K, 100M, 100G, 2T)',
        'To multiply the current limit by a multiplier, type xN. Example: x2',
        'Format: metric-name [used/limit]\n'
    ]

    for msg in messages:
        print(msg)


def yaml_to_dict(filename):
    example = {
        'service': '',
        'cloud_id': '',
        'metrics': [
            {'name': 'metric-name-1', 'limit': 'metric-limit-1'},
            {'name': 'metric-name-2', 'limit': 'metric-limit-2'},
        ]
    }
    pattern = yaml.dump(example, sort_keys=False)

    with open(filename, 'r') as infile:
        data = yaml.load(infile, Loader=yaml.Loader)
    infile.close()

    try:
        if data.get('cloud_id') is None:
            data['cloud_id'] = ''

        assert data.keys() == example.keys()
    except (AttributeError, TypeError, KeyError, AssertionError):
        raise ValidateError(f'Invalid yaml, see example. \n\n{pattern}')

    return data


def warning_message(message, attempt=3):
    print(Color.make(f'\n{message}', color='red'))
    count = 0
    try:
        while count < attempt:
            warning_msg = input('\nAre you sure? [y/n]: ')
            if warning_msg.lower() in ('n', 'no', 'q', 'quit', 'exit', 'abort'):
                sys.exit(Color.make('\nAborted!\n', color='yellow'))
            elif warning_msg.lower() in ('y', 'yes'):
                return True
            else:
                print('Not recognized. Please, type "y" or "n".')
                count += 1
    except KeyboardInterrupt:
        sys.exit(Color.make('\nYOU DIED', color='red'))

    sys.exit(Color.make('\nYOU DIED', color='red'))
