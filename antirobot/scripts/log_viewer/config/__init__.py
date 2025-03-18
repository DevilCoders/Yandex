import os


class Config(object):
    LOG_DIR = os.environ.get('LOG_DIR') or '.'
    KEYS_FILE = os.environ.get('KEYS_FILE') or 'keys'
    SPRAVKA_KEY_FILE = os.environ.get('SPRAVKA_KEY_FILE') or 'spravka_key'
    DEBUG = 1
    YT_PROXY = 'hahn.yt.yandex.net'
    YT_TOKEN = os.environ.get('YT_TOKEN')
    CLUSTER_ROOT = '//home/antirobot/log-viewer'
    SLOW_SEARCH_RUNNER = 'slow_search'
    APP_BIN_PATH = os.environ.get('APP_BIN_PATH') or '/app/bin'
