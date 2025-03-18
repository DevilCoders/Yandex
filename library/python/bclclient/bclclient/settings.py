
HOST_TEST = 'balalayka-test.paysys.yandex-team.ru'
"""Тестовый хост."""

HOST_PRODUCTION = 'balalayka.yandex-team.ru'
"""Боевой хост."""

TVM_ID_TEST = '2011732'
"""ID TVM-приложения для теста."""

TVM_ID_PRODUCTION = '2011730'
"""ID TVM-приложения для боя."""

TVM_IDS = {
    HOST_TEST: TVM_ID_TEST,
    HOST_PRODUCTION: TVM_ID_PRODUCTION,
}
"""Соответствие ID TVM-приложений хостам."""

DEFAULT_HOST = HOST_PRODUCTION
"""Хост по умолчанию. Обычно боевой."""
