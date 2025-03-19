"""Backup Item"""

import re
import logging
from datetime import datetime
from dotmap import DotMap

DATE_RE = re.compile(r"^(?P<year>20\d\d)-?(?P<month>\d\d)-?(?P<day>\d\d)$")


class BackupItem():  # pylint: disable=too-few-public-methods
    """
    BackupItem contains backup stats:
        name = entity name, like short_hostname or backup group
        type = database type
        path = full s3 path
        size = backup size
        objects = number of files in backup
    """

    supported_frequencies = ('daily', 'weekly', 'monthly')
    log = logging.getLogger(__name__)

    def __init__(self, path=None, timestamp=None):
        self.matched = None
        self.timestamp = timestamp
        self._key = None
        if path:
            try:
                path_arr = path.rstrip("/").split("/")
                # pop from path_arr last path element, expect date like "YYYY-?MM-?DD"
                self.matched = DATE_RE.match(path_arr.pop(-1))
            except IndexError:
                pass
            if self.matched:
                # mathed should contains 3 group with parsed numbers, see DATE_RE
                self.timestamp = datetime(*[int(x, 10) for x in self.matched.groups(0)])
                self.stats = DotMap({
                    "name": path_arr.pop(-1),
                    "type": path_arr.pop(-1),
                    "path": path,
                    "size": 0,
                    "objects": 0,
                })

                self.valid = 0
                self.state = "normal"
        if self.timestamp:
            self._key = DotMap(
                year=self.timestamp.year,
                month=self.timestamp.month,
                day=self.timestamp.day,
                week=self.timestamp.isocalendar()[1]  # week number
            )

    def key(self, freq):
        """Get key by frequency name"""
        key = self._key
        if freq == "weekly":
            return "{} week {}".format(key.year, key.week)
        if freq == "monthly":
            return "{} month {:02d}".format(key.year, key.month)
        return "{}-{:02d}-{:02d}".format(key.year, key.month, key.day)

    def __repr__(self):
        return str(self.stats.toDict())
    def __str__(self):
        return self.__repr__()
