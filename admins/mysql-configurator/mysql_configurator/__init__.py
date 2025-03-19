"proxy imports"

import logging
import requests
import grants
from .config import load_config, log2file
from .zookeeper import ZK
from .mysql import MySQL, MySQLError

logging.captureWarnings(True)
try:
    requests.packages.urllib3.disable_warnings()  # pylint: disable=no-member
except:   # pylint: disable=bare-except
    pass
