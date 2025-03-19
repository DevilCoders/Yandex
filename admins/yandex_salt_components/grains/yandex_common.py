import logging

log = logging.getLogger(__name__)


def environment():
    try:
        with open("/etc/yandex/environment.type", "r") as f:
            env = f.read().rstrip()
            return {"yandex-environment": env}
    except IOError as e:
        log.warning(f"failed to get Yandex environment: {e}")
        # -- raise nothing, some clusters doesn't use yandex-environment --


def dom0():
    try:
        with open("/etc/dom0hostname", "r") as f:
            dom0 = f.read().rstrip()
            return {"dom0hostname": dom0}
    except IOError:
        pass
