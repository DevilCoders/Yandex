import os
import re
from datetime import timedelta
from django.utils import timezone
import geobase3


APP_ROOT = os.path.abspath(os.path.dirname(__file__))
STORAGE_ROOT = '/place/steam/storage/'
TEMP_ROOT = '/place/steam/temp/'
ZERO_TIME = timezone.datetime.fromtimestamp(0, timezone.get_default_timezone())
DEFAULT_TASKPOOL_LIFETIME = timedelta(25)
DEFAULT_STATS_PERIOD = timedelta(30)
ASSIGNED_EST_TIMEOUT = timedelta(10)
SKIPPED_EST_TIMEOUT = timedelta(10)
TEMP_FILE_TIMEOUT = timedelta(1)
LOG_FILE = '/var/log/yandex/steam/steam.log'
ACCESS_LOG_FILE = '/var/log/nginx/access.log'
GEOBASE_FILE = '/var/cache/geobase/geodata3.bin'
LOG_LINES = 3000
GOOD_HTTP500_RATE = 0.1

#uses in check_databese in /health. Check is this variable greater then value
DATABASE_VARIABLES_CHECKER = { "wait_timeout" : 28800,
                               "max_allowed_packet" : 134217720}

HIDEREFERER_FORMAT = 'http://h.school-wiki.yandex.net/?%s'

HIDE_BOLD_RE = re.compile(r'</?b(?:\s[^>]*)?>', re.I)
BULK_CREATE_BATCH_SIZE = 100

AVATARS_URL = 'https://avatars.mds.yandex.net'

SEGMENTATION_ENABLED = False
# temporary solution for rca_standalone cache
PATH_TO_2000 = os.path.join(APP_ROOT, '2000.txt')
try:
    from local.core import *
except ImportError:
    pass

GEOBASE = geobase3.Lookup(GEOBASE_FILE)

if not os.path.isdir(TEMP_ROOT):
    os.mkdir(TEMP_ROOT)

# temporary solution for rca_standalone cache
urls_file = open(PATH_TO_2000)
URLS_LIST = [url.strip() for url in urls_file]
urls_file.close()
