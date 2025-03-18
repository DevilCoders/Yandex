from .client import Dss
from .http import HttpConnector
from .settings import HOST_TEST, HOST_PRODUCTION

VERSION = (0, 7, 0)

version_str = '.'.join(map(str, VERSION))
