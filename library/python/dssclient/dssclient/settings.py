
HOST_TEST: str = 'bosign-tst.ld.yandex.ru'
"""Тестовый хост."""

HOST_PRODUCTION: str = 'dss.yandex-team.ru'
"""Боевой хост."""

DEFAULT_HOST: str = HOST_PRODUCTION
"""Хост по умолчанию. Обычно боевой."""

AUTHORITY_OUTER: int = 1
"""Идентификатор [абстрактного] внешнего (out of band) УЦ."""

AUTHORITY_TEST: int = 2
"""Идентификатор встроенного тестового УЦ."""
