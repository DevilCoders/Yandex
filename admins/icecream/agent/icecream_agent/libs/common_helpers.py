""" Icecream agent common helper functions """
import json
import time
import random
import logging
import dotmap


def gbs(bts):
    """ Convert bytes to gigabytes """
    result = int(bts/1024/1024/1024)
    logging.debug('Converted bytes (%s) to gigabytes: %s',
                  bts, result)
    return result


def random_sleep(t_min, t_max):
    """ Random sleep """
    result_time = random.randint(t_min, t_max)
    logging.debug('Going to sleep in %s:%s range, result time: %s',
                  t_min, t_max, result_time)
    time.sleep(result_time)
    logging.debug('Good morning!')


def load_config(default, path):
    """ Load custom config and merge it with default """
    config = dotmap.DotMap(default)
    logging.debug('Loaded default config: %s', default)
    logging.debug('Loading current config from %s', path)
    with open(path) as config_file:
        custom = json.load(config_file)
    config.update(custom)
    config = config.toDict()
    logging.info('Loaded result config: %s', config)
    # reconverting dotmap, workaround for:
    # nested dict keys wont be attributes after update
    return dotmap.DotMap(config)
