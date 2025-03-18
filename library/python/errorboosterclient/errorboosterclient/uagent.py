from functools import partial
from json import dumps
from os import environ

import urllib3


class UnifiedAgentSender(object):
    """Производит отправку событий через HTTP интерфейс
    Unified Agent в Deploy.

    https://clubs.at.yandex-team.ru/infra-cloud/2587

    Чтобы работало в Deploy:
        1. Logbroker Tools >= 22.01.2022 2739742779
        2. Runtime Version >= 13

    Пример использования::

        sentry_sdk.init(
            transport=ErrorBoosterTransport(
                project='myproject',
                sender=UnifiedAgentSender(),
            ),
            ...
        )

    """
    def __init__(self, url='', timeout=None):
        # type: (str, int) -> None

        _pool = urllib3.PoolManager(
            num_pools=2,
        )

        if not url:
            url = (
                'http://%s:%s/errorbooster' % (
                    environ['ERROR_BOOSTER_HTTP_HOST'],
                    environ['ERROR_BOOSTER_HTTP_PORT']
                )
            )

        if timeout is None:
            timeout = 3

        self.request = partial(
            _pool.request, 'POST', url, timeout=timeout
        )

    def __call__(self, event):
        # type: (dict) -> None
        self.request(body=dumps(event))
