from .base import ApiResource


class Utils(ApiResource):
    """Утилиты."""

    def ping(self) -> bool:
        """Осуществляет проверку доступности. В случае удачи, вернёт True."""
        return not self._conn.request(url='ping/').errors
