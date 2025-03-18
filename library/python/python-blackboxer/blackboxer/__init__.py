import platform

from .__version__ import __version__  # noqa
from .blackboxer import Blackbox  # noqa
from .environment import ENV, URL, url_from_yenv  # noqa
from .exceptions import (  # noqa
    AccessDenied, BlackboxError, ConnectionError, FieldRequiredError,  # noqa
    HTTPError, InvalidParamsError, ResponseError, TemporaryError,  # noqa
    TransportError, UnknownError  # noqa
)

if platform.python_version_tuple()[0] == "3":
    from ._async import AsyncBlackbox  # noqa
