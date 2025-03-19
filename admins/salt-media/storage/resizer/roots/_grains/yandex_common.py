import logging

log = logging.getLogger(__name__)


class EnvironmentError(Exception):
    pass


def environment():
    try:
        with open("/etc/yandex/environment.type", 'r') as f:
            env = f.read().rstrip()
            return {'yandex-environment': env}
    except IOError as e:
        log.warning("Failed to get Yandex environment")
        # -- raise nothing, some clusters doesn't use yandex-environment --
        # raise EnvironmentError("failed to get yandex environment")


def dom0():
    dom0 = ""
    try:
        with open('/etc/dom0hostname', 'r') as f:
            dom0 = f.read().rstrip()
    except IOError as e:
        return

    result = {'dom0hostname': dom0}
    return result
