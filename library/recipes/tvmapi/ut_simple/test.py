import os
import os.path
import requests

import ticket_parser2 as tp2


TVMAPI_PORT_FILE = "tvmapi.port"


def _get_tvmapi_port():
    with open(TVMAPI_PORT_FILE) as f:
        return int(f.read())


def test_tvmapi():
    assert os.path.isfile(TVMAPI_PORT_FILE)

    port = _get_tvmapi_port()

    r = requests.get("http://localhost:%d/2/keys?lib_version=100500" % port)
    assert r.status_code == 200, r.text

    cs = tp2.TvmApiClientSettings(
        self_client_id=1000501,
        self_secret='bAicxJVa5uVY7MjDlapthw',
        dsts={'my backend': 1000502},
        enable_service_ticket_checking=True,
    )
    cs.__set_localhost(port)

    ca = tp2.TvmClient(cs)
    assert ca.status == tp2.TvmClientStatus.Ok

    st = ca.check_service_ticket(
        '3:serv:CBAQ__________9_IgYIexC1iD0:QQkak4KsSImXYJmj3wfXB_4XNBMe067AT0FUYN-dYxrWHAq2YoZsVQ_8p_sBGs50KRKuHxaXlJtfHLP7Dfrpk9tui0itcnTZdL4n1Ly0W9T1M3r5YPqi5a3LCq1bh6nnPv5JwVSTjUs5yzYSCF2ucRjXUPZA8orB91Ts8xGqhYM'  # noqa
    )
    assert st.src == 123

    ca.stop()
