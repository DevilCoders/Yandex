import os
import logging

from flask import Flask
from flask_bootstrap import Bootstrap

from newcachelib import CacheManager
from .util import DateEncoder

from raven.contrib.flask import Sentry


app = Flask(__name__)
app.config["SECRET_KEY"] = "DUMMY_KEY"
app.json_encoder = DateEncoder

REDIS_DB = int(os.environ.get('REDIS_DB', "3"))
REDIS_SENTINELS = os.environ.get(
    'REDIS_SENTINELS',
    ','.join([
        'man1-8872.search.yandex.net:21750',
        'myt1-2531.search.yandex.net:21750',
        'sas1-3116.search.yandex.net:21750',
    ])
)

cache_manager = CacheManager.create_from_sentinels_string(REDIS_SENTINELS, redis_db=REDIS_DB)
Bootstrap(app)

FORMAT = "%(asctime)s:%(levelname)s:%(name)s:%(message)s"
logging.basicConfig(level=logging.DEBUG, format=FORMAT)

from . import gencfg_graphs, gencfg_volumes_graphs, test_gencfg_graphs, hosts_anomalies, abc, openstack, metaprj, total_alloc, dashboard  # NOQA

if 'SENTRY_DSN' in os.environ:
    sentry = Sentry(app, dsn=os.environ['SENTRY_DSN'], logging=True, level=logging.WARNING)
