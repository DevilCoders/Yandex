from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from yc_common import logging
from yc_common.clients.common import ServiceHealth
from yc_common.clients.kikimr import KikimrError

log = logging.get_logger(__name__)


class Check(object):
    _name = None  # type: str
    _type = None  # type: str
    _critical = False

    def __init__(self):
        self.state = ServiceHealth.Status.ERROR

    def __call__(self):
        self.state = ServiceHealth.Status.OK

    @property
    def id(self):
        return self._type + ":" + self._name

    @property
    def is_critical(self):
        return self._critical


class DatabaseCheck(Check):
    _name = "marketplace"
    _type = "database"
    _critical = True

    def __call__(self):
        query = "SELECT 1"
        try:
            marketplace_db(request_timeout=10, retry_timeout=5).query(query)
            self.state = ServiceHealth.Status.OK
        except KikimrError as err:
            log.error("Marketplace database check failed: %s", err)
            self.state = ServiceHealth.Status.ERROR


ALL_CHECKS = (DatabaseCheck(),)
