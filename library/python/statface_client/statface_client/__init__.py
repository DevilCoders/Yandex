from __future__ import division, absolute_import, print_function, unicode_literals
import logging as _logging

from .interface import *
from .errors import *
from .constants import *
from .version import __version__

_logging.getLogger('statface_client').addHandler(_logging.NullHandler())
