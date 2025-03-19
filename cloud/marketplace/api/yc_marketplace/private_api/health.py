import json
import time

from yc_common import config
from yc_common import exceptions
from yc_common import logging
from yc_common.clients.common import ServiceHealth
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.checks import ALL_CHECKS
from cloud.marketplace.common.yc_marketplace_common.models.health import HealthModel

log = logging.get_logger(__name__)

_Status = ServiceHealth.Status
_MIN_HEALTH_CHECK_PROBE_INTERVAL = 30


class Health(object):
    def __init__(self):
        self._checks = []
        self._health = {
            "health": _Status.OK,
            "code": ServiceHealth.code_from_status(_Status.OK),
            "timestamp": 0,
        }
        self._interval = None

    def is_actual(self):
        ts_diff = time.time() - self._health["timestamp"]
        if not self._interval:
            self._interval = config.get_value(
                "health.check_interval",
                default=_MIN_HEALTH_CHECK_PROBE_INTERVAL,
            )
        return ts_diff < self._interval

    @property
    def health(self):
        return self._health["health"]

    @health.setter
    def health(self, health_value):
        self._health = {
            "health": health_value,
            "code": ServiceHealth.code_from_status(health_value),
            "timestamp": time.time(),
        }

    @property
    def code(self):
        return self._health["code"]

    def calculate_health(self):
        new_health = _Status.OK
        for check in self._checks:
            if check.state == ServiceHealth.Status.ERROR:
                if check.is_critical:
                    new_health = ServiceHealth.Status.ERROR
                    break
                else:
                    new_health = _Status.WARNING
        self.health = new_health

    def list_checks(self):
        return self._checks

    def _do(self, checks):
        for check in checks:
            try:
                check()
            except BaseException as err:
                log.exception("%s check has crashed %s", check.id, err)
                check.state = ServiceHealth.Status.ERROR

            self._checks.append(check)

    def do_checks(self):
        # FIXME: Control check execution time.
        self._checks = []

        self._do(ALL_CHECKS)

        self.calculate_health()


health = Health()


@private_api_handler("GET", "/health", params_model=HealthModel)
def health_check(request, **kwargs):
    if not health.is_actual():
        health.do_checks()

    monrun = request.get("monrun", False)

    if not monrun:
        result = {check.id: check.state for check in health.list_checks()}
        return {"health": health.health, "details": result}
    else:
        result = {check.id: check.state for check in health.list_checks() if check.state != ServiceHealth.Status.OK}
        message = json.dumps(result) if result else "OK"
        return "marketplace;" + health.code + ";" + message


@private_api_handler("GET", "/ping")
def ping(**kwargs):
    if not health.is_actual():
        health.do_checks()

    if health.health == _Status.ERROR:
        raise exceptions.InternalServerError()

    return {"ping": "ok"}
