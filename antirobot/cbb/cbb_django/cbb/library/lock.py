import ylock

from django.conf import settings


if settings.YLOCK["prefix"] != "":
    manager = ylock.backends.create_manager(**settings.YLOCK)
else:
    manager = None


class LockedCommandMixin(object):
    """
    For zookeeper (ylock) purpose
    """
    def handle(self, *args, **kwargs):
        if manager is None:
            return

        with manager.lock(self.__module__, block=False) as is_locked:
            if not is_locked:
                self.stdout.write("Command execution is already locked")
                return

            return self._handle(*args, **kwargs)
