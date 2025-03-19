import typing as tp
import os
import logging
from clan_tools.data_adapters.ClickHouseRestAdapter import ClickHouseRestAdapter

logger = logging.getLogger(__name__)


class ClickHouseAdapter(ClickHouseRestAdapter):
    """
    To get SSL certificate run on your machine:

    mkdir -p /usr/local/share/ca-certificates/Yandex && \
    wget "https://crls.yandex.net/allCAs.pem" -O /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt

    """

    def __init__(self, user: tp.Optional[str] = None, password: tp.Optional[str] = None,
                 host: str = 'https://c-07bc5e8c-c4a7-4c26-b668-5a1503d858b9.rw.db.yandex.net:8443',
                 verify: str = '/usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt', timeout: int = 600) -> None:
        if user is None:
            user = os.environ['CH_USER']
        if password is None:
            password = os.environ['CH_PASSWORD']
        self._post_conf = dict(url=host, timeout=timeout, auth=(user, password), verify=verify)

__all__ = ['ClickHouseAdapter']
