from yc_common.exceptions import LogicalError
from yc_common.models import Model, StringType


class ServiceHealth(Model):
    class Status:
        OK = "ok"
        WARNING = "warning"
        ERROR = "error"

    class StatusCode:
        """Monrun status codes."""
        OK = "0"
        WARNING = "1"
        ERROR = "2"

    health = StringType(required=True)

    @classmethod
    def code_from_status(cls, status: str):
        mapping = {
            cls.Status.OK: cls.StatusCode.OK,
            cls.Status.WARNING: cls.StatusCode.WARNING,
            cls.Status.ERROR: cls.StatusCode.ERROR,
        }
        try:
            return mapping[status]
        except KeyError:
            raise LogicalError("Unknown status: {}", status)
