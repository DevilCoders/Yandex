# noqa: F401
# DEVTOOLSSUPPORT-3733: there is no way to disable exactly one warning F401 completely for one file

import sys

if sys.version_info < (2, 7, 3):
    raise Exception("Please install at least Python 2.7.3 (or just use Anaconda).")

from .mstand_def_values import OfflineDefaultValues

__all__ = [
    "OfflineDefaultValues",
]
