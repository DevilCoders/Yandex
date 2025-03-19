import pytest
import responses
from responses.registries import OrderedRegistry

from dataproc_init_actions import send_state


class TestRetries:
    @responses.activate(registry=OrderedRegistry)
    def test_retry_on_RequestException(self):
        responses.post(
            "http://masternode:5555/initacts",
            json={"msg": "OK"},
            status=200,
        )
        responses.post(
            "http://masternode:5555/initacts",
            json={"msg": "internal error"},
            status=500,
        )
        responses.post(
            "http://masternode:5555/initacts",
            json={"msg": "OK"},
            status=200,
        )
        assert send_state("SUCCESSFUL", "fqdn", "masternode").status_code == 200
        assert send_state("SUCCESSFUL", "fqdn", "masternode").status_code == 200

    @responses.activate(registry=OrderedRegistry)
    def test_do_not_retry_on_not_RequestException(self):
        responses.post("http://masternode:5555/initacts", body=IOError())
        with pytest.raises(IOError):
            send_state("SUCCESSFUL", "fqdn", "masternode")
