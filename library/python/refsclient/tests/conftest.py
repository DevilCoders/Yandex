import pytest

from refsclient import Refs, HOST_TEST


@pytest.fixture
def refs():

    def refs_(local=False):
        obj = Refs(host='localhost:8000' if local else HOST_TEST, timeout=15)

        if local:
            obj._connector._url_base = obj._connector._url_base.replace('https', 'http')

        return obj

    return refs_
