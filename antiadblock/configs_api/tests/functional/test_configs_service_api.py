from copy import deepcopy

import pytest
from hamcrest import assert_that, has_item

from conftest import VALID_CONFIG, USER2_SESSION_ID, Session


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_get_config_status_403(api, service, status):
    session = Session(USER2_SESSION_ID)
    label_id = service["id"]
    r = session.get(api["label"][label_id]["config"][status])
    assert r.status_code == 403


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_get_config(api, service, tickets, status):
    session = Session(USER2_SESSION_ID)
    label_id = service["id"]

    r = session.get(api["label"][label_id]["config"][status], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), VALID_CONFIG)


@pytest.mark.usefixtures("transactional")
def test_create_config(api, service, tickets):
    session = Session(USER2_SESSION_ID)
    label_id = service["id"]

    r = session.get(api["label"][label_id]["config"]["test"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    config = r.json()

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]),
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})

    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0
    assert_that(r.json()["data"]["CRYPT_URL_RE"], has_item('/static/.*'))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_activate_config(api, service, tickets, status):
    session = Session(USER2_SESSION_ID)
    label_id = service["id"]

    r = session.get(api["label"][label_id]["config"][status], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    config = r.json()
    old_config_id = config["id"]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config_id),
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201
    config_id = r.json()["id"]

    r = session.put(api["label"][label_id]["config"][config_id][status],
                    json=dict(old_id=old_config_id),
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})

    assert r.status_code == 200
