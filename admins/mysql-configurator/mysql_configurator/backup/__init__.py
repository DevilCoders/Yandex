"proxy imports"

import logging
import requests
from .lock import Lock, DummyLock
from .worker import Worker
from .utils import stopwatch, check_output
from .pusher import S3Pusher, FTPPusher

logging.captureWarnings(True)
try:
    requests.packages.urllib3.disable_warnings()  # pylint: disable=no-member
except:   # pylint: disable=bare-except
    pass
