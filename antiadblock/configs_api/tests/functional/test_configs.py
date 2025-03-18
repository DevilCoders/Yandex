# coding=utf-8
from copy import deepcopy

import pytest
from hamcrest import has_entries, is_, any_of, assert_that, contains, only_contains, contains_inanyorder, empty, \
    has_entry, equal_to
import jwt

from conftest import ADMIN_USER_ID, DEFAULT_PUBLIC_KEY, VALID_CONFIG, VALID_CONFIG_DATA


@pytest.mark.usefixtures("transactional")
def test_create_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["service"][service_id])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(name="autoru",
                                      domain="auto.ru",
                                      id="auto.ru",
                                      status=any_of("wip", "ok", "inactive", "error"),
                                      owner_id=ADMIN_USER_ID))
    assert 'autoru' == r.json()["name"]


@pytest.mark.usefixtures("transactional")
def test_cant_create_invalid_service(api, session):
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="    "))
    assert response.status_code == 400

    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="   ",
                                                      domain="yandex.ru"))
    assert response.status_code == 400

    response = session.post(api['service'], json=dict(service_id="   ",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 400

    response = session.post(api['service'], json=dict(service_id="yandex_ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_cant_create_service_with_same_domain(api, session):
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 201

    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_get_services(api, session):
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 201
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    r = session.get(api["services"])
    assert r.status_code == 200
    # should be ordered by name
    assert_that(r.json(), has_entries(items=contains(has_entries(name="autoru"),
                                                     has_entries(name="morda")),
                                      total=is_(int)))


@pytest.mark.usefixtures("transactional")
def test_get_service_not_exists(api, session):
    r = session.get(api["service"][123123123])
    assert r.status_code == 404


@pytest.mark.usefixtures("transactional")
def test_get_config_not_exists(api, session, service):
    r = session.get(api["service"][service["id"]][123123123])
    assert r.status_code == 404


@pytest.mark.usefixtures("transactional")
def test_first_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]
    assert_that(config, VALID_CONFIG)

    assert "auto\\.ru" in config["data"]["PROXY_URL_RE"][0]
    payload = jwt.decode(config["data"]["PARTNER_TOKENS"][0], DEFAULT_PUBLIC_KEY, algorithms="RS256")
    assert payload["iss"] == "AAB"
    assert payload["sub"] == service_id


@pytest.mark.usefixtures("transactional")
def test_first_config_active(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    assert r.json()["id"] == config["id"]


@pytest.mark.usefixtures("transactional")
def test_create_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.get(api["config"][config["id"]])
    assert r.status_code == 200
    assert r.json()["id"] == config["id"]
    assert '/static/.*' in "".join(r.json()["data"]["CRYPT_URL_RE"])


@pytest.mark.usefixtures("transactional")
def test_config_limit_offset(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    parent_id = config["id"]
    for i in xrange(10):
        r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config {}".format(i), parent_id=parent_id))
        assert r.status_code == 201
        parent_id = r.json()["id"]

    r = session.get(api["label"][service_id]["configs"], params=dict(offset=5, limit=6))
    assert r.status_code == 200
    assert r.json()["total"] == 11
    assert len(r.json()["items"]) == 6
    assert r.json()["items"][5]["id"] == config["id"]


@pytest.mark.usefixtures("transactional")
def test_activate_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_activate_already_activated(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    r = session.put(api["label"][service_id]["config"][old_config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(id=old_config["id"],
                                      statuses=contains_inanyorder(
                                      has_entries(status="active"),
                                      has_entries(status="test"),
                                      has_entries(status="approved"),
                                      )))


@pytest.mark.usefixtures("transactional")
def test_config_activate_rush(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    # Creating two simultaneous config
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config1 = r.json()

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config2 = r.json()

    # and simulate simultaneous activating
    r = session.put(api["label"][service_id]["config"][config1["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config2["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 409

    # check that only one config was activated
    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(id=config1["id"],
                                      statuses=contains_inanyorder(
                                      has_entries(status="active"),
                                      has_entries(status="approved"),
                                      )))

    r = session.get(api["config"][config2["id"]])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(id=config2["id"],
                                      statuses=empty()))


@pytest.mark.usefixtures("transactional")
def test_try_activate_config_different_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    # create another service
    r = session.post(api['service'], json=dict(service_id="service.ru",
                                               name="another-well-known-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service = r.json()

    # create config for another service
    r = session.get(api["label"][another_service["id"]]["config"]["active"])
    assert r.status_code == 200
    another_old_config = r.json()

    config_data = deepcopy(another_old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][another_service["id"]]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=another_old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    # and try activate another config for our service
    r = session.put(api["label"][service_id]["config"][config["id"]]["active"],
                    json=dict(old_id=old_config["id"]))
    assert r.status_code == 404

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(id=old_config["id"],
                                      statuses=contains_inanyorder(
                                      has_entries(status="active"),
                                      has_entries(status="test"),
                                      has_entries(status="approved"),
                                      )))


@pytest.mark.usefixtures("transactional")
def test_test_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        ))))

    r = session.get(api["label"][service_id]["config"]["test"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(id=config["id"],
                                      statuses=contains_inanyorder(
                                      has_entries(status="test"),
                                      )))


@pytest.mark.usefixtures("transactional")
def test_test_two_configs(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    config_data["CRYPT_URL_RE"].append(r'/dynamic/.*')
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 3", parent_id=config["id"]))
    assert r.status_code == 201
    config2 = r.json()
    r = session.put(api["label"][service_id]["config"][config2["id"]]["test"], json=dict(old_id=config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 3
    assert_that(r.json()["items"], contains(has_entries(id=config2["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        )),
                                            has_entries(id=config["id"],
                                                        statuses=empty()),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_test_concurrent_configs(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    # Create two configs with the same parent
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config2 = r.json()
    # Move first to testing
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # Try move second to testing, simulating simultaneity (old_id=None)
    r = session.put(api["label"][service_id]["config"][config2["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 409


@pytest.mark.usefixtures("transactional")
def test_test_concurrent_configs2(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    # Create config and move it to test
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # Create another 2 configs
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 3", parent_id=config["id"]))
    assert r.status_code == 201
    config2 = r.json()
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 3", parent_id=config["id"]))
    assert r.status_code == 201
    config3 = r.json()

    # Move first to testing
    r = session.put(api["label"][service_id]["config"][config2["id"]]["test"], json=dict(old_id=config["id"]))
    assert r.status_code == 200

    # Try move second to testing, simulating simultaneity (old_id=config["id"])
    r = session.put(api["label"][service_id]["config"][config3["id"]]["test"], json=dict(old_id=config["id"]))
    assert r.status_code == 409


@pytest.mark.usefixtures("transactional")
def test_activate_test_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=only_contains(
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_try_test_config_different_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    # create another service
    r = session.post(api['service'], json=dict(service_id="service.ru",
                                               name="another-well-known-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service = r.json()

    # create config for another service
    r = session.get(api["label"][another_service["id"]]["config"]["active"])
    assert r.status_code == 200
    another_old_config = r.json()

    config_data = deepcopy(another_old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][another_service["id"]]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=another_old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    # and try test another config for our service
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"],
                    json=dict(old_id=old_config["id"]))
    assert r.status_code == 404

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(total=1,
                                      items=contains(has_entries(id=old_config["id"],
                                                                 statuses=contains_inanyorder(
                                                                 has_entries(status="active"),
                                                                 has_entries(status="test"),
                                                                 has_entries(status="approved"),
                                                                 )))))


@pytest.mark.usefixtures("transactional")
def test_archive_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    assert_that(r.json()["items"], contains(has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_recover_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.patch(api["config"][config["id"]], json=dict(archived=False))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains_inanyorder(has_entries(id=config["id"],
                                                                   statuses=empty()),
                                                       has_entries(id=old_config["id"],
                                                                   statuses=contains_inanyorder(
                                                                   has_entries(status="active"),
                                                                   has_entries(status="test"),
                                                                   has_entries(status="approved"),
                                                                   ))))


@pytest.mark.usefixtures("transactional")
def test_cant_activate_archived_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.patch(api["config"][old_config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][old_config["id"]]["active"], json=dict(old_id=config["id"]))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_cant_mark_test_archived_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_get_archived_configs(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"], params=dict(show_archived=True))
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=empty(),
                                                        archived=True),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ),
                                                        archived=False)))


@pytest.mark.usefixtures("transactional")
def test_limit_offset_with_archived(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config1 = r.json()

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 3", parent_id=old_config["id"]))
    assert r.status_code == 201
    config2 = r.json()

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 4", parent_id=old_config["id"]))
    assert r.status_code == 201

    r = session.patch(api["config"][config2["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"], params=dict(show_archived=True, offset=1, limit=2))
    assert r.status_code == 200
    assert r.json()["total"] == 4
    assert_that(r.json()["items"], contains(has_entries(id=config2["id"],
                                                        statuses=empty(),
                                                        archived=True),
                                            has_entries(id=config1["id"],
                                                        statuses=empty(),
                                                        archived=False)))


@pytest.mark.usefixtures("transactional")
def test_cant_archive_active_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_cant_archive_test_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_can_set_service_inactive(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200
    assert r.json()["status"] == "inactive"


@pytest.mark.usefixtures("transactional")
def test_cant_set_service_inactive(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_cant_set_service_active(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["enable"])
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_can_set_service_active(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.post(api["service"][service_id]["enable"])
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


@pytest.mark.usefixtures("transactional")
def test_cant_set_service_not_exists_status(api, session):
    r = session.post(api["service"]["123123123"]["disable"])
    assert r.status_code == 404

    r = session.post(api["service"]["123123123"]["enable"])
    assert r.status_code == 404


@pytest.mark.usefixtures("transactional")
def test_create_config_inactive_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_archive_config_inactive_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][service_id]["config"], json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_activate_config_inactive_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_test_config_inactive_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_generate_token(api, session, service):
    service_id = service["id"]
    r = session.get(api["service"][service_id]["gen_token"])

    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        "token": is_(basestring),
    }))
    token = r.json()["token"]
    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["PARTNER_TOKENS"].append(token)

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_activate_approve_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=True))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_cant_activate_decline_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"], json=dict(approved=False, comment="Declined"))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="declined"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_adm_activate_not_moderated_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_test_approve_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"], json=dict(approved=True, comment="Approved"))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_cant_test_decline_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"], json=dict(approved=False, comment="Declined"))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="declined"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_test_not_moderated_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
def test_cant_moderate_active_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=False, comment="Declined"))
    assert r.status_code == 400

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=True, comment="Approved"))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_moderate_test_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=False, comment="Declined"))
    assert r.status_code == 200

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=True, comment="Approved"))
    assert r.status_code == 200


@pytest.mark.usefixtures("transactional")
def test_moderate_moderated_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=False))
    assert r.status_code == 400

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=False, comment="Declined"))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="declined", comment="Declined"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=False, comment="New declined"))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="declined", comment="New declined"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=True))
    assert r.status_code == 200

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=True))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_moderate_moderated_config_2(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"],
                    json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="approved"),
                                                        ))))

    r = session.patch(api["label"][service_id]["config"][old_config["id"]]["moderate"],
                      json=dict(approved=False, comment="New declined"))
    assert r.status_code == 200

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 2
    assert_that(r.json()["items"], contains(has_entries(id=config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="active"),
                                                        has_entries(status="approved"),
                                                        )),
                                            has_entries(id=old_config["id"],
                                                        statuses=contains_inanyorder(
                                                        has_entries(status="test"),
                                                        has_entries(status="declined", comment="New declined"),
                                                        ))))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("property_name", ["monitorings_enabled", "mobile_monitorings_enabled"])
def test_service_change_monitorings_enabled(api, session, service, property_name):
    service_id = service["id"]
    r = session.post(api["service"][service_id]["set"],
                     json={property_name: True})
    assert r.status_code == 200

    changed_service = session.get(api["service"][service_id])
    assert_that(changed_service.json(), has_entry(property_name, True))

    r = session.post(api["service"][service_id]["set"],
                     json={property_name: False})
    assert r.status_code == 200

    changed_service = session.get(api["service"][service_id])
    assert_that(changed_service.json(), has_entry(property_name, False))


@pytest.mark.usefixtures("transactional")
def test_cant_set_service_property_no_data(api, session, service):
    service_id = service["id"]
    r = session.post(api["service"][service_id]["set"])
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('comment', ['', 'new comment', 'new\ncomment', 'new\\ncomment', 'select 1;', 'window.alert(1);'])
def test_service_add_comment(api, session, service, comment):
    service_id = service["id"]
    r = session.post(api['service'][service_id]["comment"],
                     json=dict(comment=comment))
    assert r.status_code == 200

    r = session.get(api['service'][service_id]["comment"])
    assert r.status_code == 200
    assert r.json()["comment"] == comment


@pytest.mark.usefixtures("transactional")
def test_service_add_two_comment(api, session, service):
    service_id = service["id"]
    comment1 = "comment1"
    r = session.post(api['service'][service_id]["comment"],
                     json=dict(comment=comment1))
    assert r.status_code == 200

    r = session.get(api['service'][service_id]["comment"])
    assert r.status_code == 200
    assert r.json()["comment"] == comment1

    comment2 = "comment2"
    r = session.post(api['service'][service_id]["comment"],
                     json=dict(comment=comment2))
    assert r.status_code == 200

    r = session.get(api['service'][service_id]["comment"])
    assert r.status_code == 200
    assert r.json()["comment"] == comment2


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('current, whitelist, deprecated, expected',
                         (('', [], ['other_cookie'], 400),
                          ('new_cookie', ['new_cookie', 'other_cookie'], ['other_cookie'], 201),
                          ('new_cookie', ['other_cookie'], [], 400),
                          ('new_cookie', [], [], 400),
                          ('new_cookie', [], ['new_cookie', 'other_cookie'], 400),
                          ('new_cookie', ['new_cookie'], ['new_cookie'], 400),
                          )
                         )
def test_add_current_cookie(api, session, service, current, whitelist, deprecated, expected):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CURRENT_COOKIE"] = current
    config_data["DEPRECATED_COOKIES"] = deprecated
    config_data["WHITELIST_COOKIES"] = whitelist

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == expected


@pytest.mark.usefixtures("transactional")
def test_create_and_get_labels(api, session, service):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_2"))
    assert r.status_code == 201
    r = session.post(api["label"]["auto.ru"]["change_parent_label"], json=dict(parent_label_id="label_1"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="auto.ru", label_id="mobile"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="auto.ru", label_id="desktop"))
    assert r.status_code == 201
    # create another service
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru",
                                                      parent_label_id="label_2"))
    assert response.status_code == 201

    r = session.get(api["labels"], params=dict(service_id="auto.ru"))
    assert r.status_code == 200

    expected = {
        'ROOT': {
            'label_1': {
                'auto.ru': {
                    'desktop': {},
                    'mobile': {}
                }
            },
            'label_2': {},
        }
    }
    assert r.json() == expected

    r = session.get(api["labels"], params=dict(service_id="yandex.ru"))
    assert r.status_code == 200

    expected = {
        'ROOT': {
            'label_1': {},
            'label_2': {
                'yandex.ru': {}
            },
        }
    }
    assert r.json() == expected


@pytest.mark.usefixtures("transactional")
def test_get_labels_many_config(api, session, service):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201
    r = session.post(api["label"]["auto.ru"]["change_parent_label"], json=dict(parent_label_id="label_1"))
    assert r.status_code == 201

    # make configs for every level
    for label_id in ("ROOT", "label_1", "auto.ru"):
        r = session.get(api["label"][label_id]["configs"])
        assert r.status_code == 200
        config = r.json()["items"][0]

        r = session.post(api["label"][label_id]["config"], json=dict(data=config["data"], data_settings={}, comment="Config 2", parent_id=config["id"]))
        assert r.status_code == 201

        r = session.get(api["label"][label_id]["configs"])
        assert r.status_code == 200
        assert r.json()["total"] == 2

    r = session.get(api["labels"], params=dict(service_id="auto.ru"))
    assert r.status_code == 200

    expected = {
        'ROOT': {
            'label_1': {
                'auto.ru': {}
            },
        }
    }
    assert r.json() == expected


@pytest.mark.usefixtures("transactional")
def test_create_exist_label(api, session):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_get_label_not_exists_service(api, session):
    r = session.get(api["labels"], params=dict(service_id="some_not_exists_service"))
    assert r.status_code == 404


@pytest.mark.usefixtures("transactional")
def test_create_service_on_label(api, session, label):
    r = session.post(api['service'], json=dict(service_id="yandex.ru",
                                               parent_label_id="label_1",
                                               name="morda",
                                               domain="yandex.ru"))
    assert r.status_code == 201

    r = session.get(api["labels"], params=dict(service_id="yandex.ru"))
    assert r.status_code == 200
    expected = {
        'ROOT': {
            'label_1': {
                'yandex.ru': {}
            },
        }
    }
    assert r.json() == expected


@pytest.mark.usefixtures("transactional")
def test_change_parent_label(api, session, service):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201

    r = session.get(api["labels"], params=dict(service_id="auto.ru"))
    assert r.status_code == 200
    expected = {
        'ROOT': {
            'label_1': {},
            'auto.ru': {},
        }
    }
    assert r.json() == expected

    r = session.post(api["label"]["auto.ru"]["change_parent_label"], json=dict(parent_label_id="label_1"))
    assert r.status_code == 201

    r = session.get(api["labels"], params=dict(service_id="auto.ru"))
    assert r.status_code == 200
    expected = {
        'ROOT': {
            'label_1': {
                'auto.ru': {}
            },
        }
    }
    assert r.json() == expected


@pytest.mark.usefixtures("transactional")
def test_cant_change_root_parent_label(api, session):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201

    r = session.post(api["label"]["ROOT"]["change_parent_label"], json=dict(parent_label_id="label_1"))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_cant_change_parent_label_loop(api, session):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="label_1", label_id="label_2"))
    assert r.status_code == 201

    r = session.post(api["label"]["label_1"]["change_parent_label"], json=dict(parent_label_id="label_2"))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_create_label_config(api, session, label):
    label_id = label["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    r = session.get(api["config"][config["id"]])
    assert r.status_code == 200
    assert r.json()["id"] == config["id"]
    assert '/static/.*' in "".join(r.json()["data"]["CRYPT_URL_RE"])


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_parent_data_in_config(api, session, service, status):
    r = session.post(api["label"], json=dict(parent_label_id="auto.ru", label_id="label_1"))
    assert r.status_code == 201

    r = session.get(api["label"]["label_1"]["configs"], params=dict(status=status))
    assert r.status_code == 200
    config = r.json()["items"][0]

    r = session.get(api["config"][config["id"]])
    assert r.status_code == 200
    assert "parent_data" in r.json()
    assert_that(r.json()["parent_data"], VALID_CONFIG_DATA)


@pytest.mark.usefixtures("transactional")
def test_label_config_limit_offset(api, session, label):
    label_id = label["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    parent_id = config["id"]
    for i in xrange(10):
        r = session.post(api["label"][label_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config {}".format(i), parent_id=parent_id))
        assert r.status_code == 201
        parent_id = r.json()["id"]

    r = session.get(api["label"][label_id]["configs"], params=dict(offset=5, limit=6))
    assert r.status_code == 200
    assert r.json()["total"] == 11
    assert len(r.json()["items"]) == 6
    assert r.json()["items"][5]["id"] == config["id"]


@pytest.mark.usefixtures("transactional")
def test_archive_and_restore_config(api, session, label):
    label_id = label["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    # make new config
    config_data = deepcopy(config["data"])
    r = session.post(api["label"][label_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0

    # trying archive and restore config
    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200
    r = session.patch(api["config"][config["id"]], json=dict(archived=False))
    assert r.status_code == 200


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_cant_archive_config_with_status(api, session, label, service, status):
    label_id = label["id"]
    service_id = service["id"]
    r = session.post(api["label"][service_id]["change_parent_label"], json=dict(parent_label_id=label_id))
    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    # make new config
    config_data = deepcopy(old_config["data"])
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0
    # mark config
    r = session.put(api["label"][service_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # trying archive with status
    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_cant_mark_experiment_config_with_status(api, session, label, service, status):
    label_id = label["id"]
    service_id = service["id"]
    r = session.post(api["label"][service_id]["change_parent_label"], json=dict(parent_label_id=label_id))
    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    # make new config
    config_data = deepcopy(old_config["data"])
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0
    # mark config
    r = session.put(api["label"][service_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200
    # trying mark experiment with status
    r = session.patch(api["config"][config["id"]]["experiment"], json=dict(exp_id="test123"))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_cant_mark_experiment_declined_config(api, session, label, service):
    label_id = label["id"]
    service_id = service["id"]
    r = session.post(api["label"][service_id]["change_parent_label"], json=dict(parent_label_id=label_id))
    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    # make new config
    config_data = deepcopy(old_config["data"])
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    assert len(config["statuses"]) == 0
    # declined config
    r = session.patch(api["label"][label_id]["config"][config["id"]]["moderate"], json=dict(approved=False, comment="decline config"))
    assert r.status_code == 200
    # trying mark experiment with status
    r = session.patch(api["config"][config["id"]]["experiment"], json=dict(exp_id="test123"))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_mark_experiment_config(api, session, label, service):
    label_id = label["id"]
    service_id = service["id"]
    r = session.post(api["label"][service_id]["change_parent_label"], json=dict(parent_label_id=label_id))
    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    # make new two configs
    config_data = deepcopy(config["data"])
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config_1 = r.json()
    config_data = deepcopy(config_1["data"])
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config_1["id"]))
    assert r.status_code == 201
    config_2 = r.json()
    # mark configs
    r = session.patch(api["config"][config_1["id"]]["experiment"], json=dict(exp_id="test_1"))
    assert r.status_code == 200
    r = session.patch(api["config"][config_2["id"]]["experiment"], json=dict(exp_id="test_2"))
    assert r.status_code == 200
    # configs have different exp_id
    r = session.get(api["label"][service_id]["configs"])
    config_1 = r.json()["items"][1]
    config_2 = r.json()["items"][0]
    assert config_1["exp_id"] == "test_1"
    assert config_2["exp_id"] == "test_2"

    r = session.patch(api["config"][config_1["id"]]["experiment"], json=dict(exp_id="test_2"))
    assert r.status_code == 200
    # config_1 have exp_id, config_2 haven't exp_id
    r = session.get(api["label"][service_id]["configs"])
    config_1 = r.json()["items"][1]
    config_2 = r.json()["items"][0]
    assert config_1["exp_id"] == "test_2"
    assert config_2["exp_id"] is None

    r = session.patch(api["config"][config_1["id"]]["experiment"]["remove"])
    assert r.status_code == 200
    # configs haven't exp_id
    r = session.get(api["label"][service_id]["configs"])
    config_1 = r.json()["items"][1]
    config_2 = r.json()["items"][0]
    assert config_1["exp_id"] is None
    assert config_2["exp_id"] is None


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_cant_activate_experiment_config(api, session, service, status):
    service_id = service["id"]
    # create new config
    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # mark experiment
    r = session.patch(api["config"][config["id"]]["experiment"], json=dict(exp_id="test_1"))
    assert r.status_code == 200
    # can't mark status
    r = session.put(api["label"][service_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_get_empty_list_experiment(api, service, session):
    response = session.get(api["label"][service["id"]]["experiment"])
    assert response.status_code == 200
    assert_that(response.json(), equal_to({"items": [], "total": 0}))


@pytest.mark.usefixtures("transactional")
def test_get_list_experiment(api, service, session):
    # make hierarchical configs
    service_id = service["id"]
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="external"))
    assert r.status_code == 201
    r = session.post(api["label"][service_id]["change_parent_label"], json=dict(parent_label_id="external"))
    assert r.status_code == 201
    # make exp config for ROOT, external
    for _label in ("ROOT", "external"):
        r = session.get(api["label"][_label]["configs"])
        assert r.status_code == 200
        old_config = r.json()["items"][0]
        config_data = deepcopy(old_config["data"])
        r = session.post(api["label"][_label]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
        assert r.status_code == 201
        config = r.json()
        r = session.patch(api["config"][config["id"]]["experiment"], json=dict(exp_id="test_{}".format(_label)))
        assert r.status_code == 200
    response = session.get(api["label"][service_id]["experiment"])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(items=only_contains("test_ROOT", "test_external"), total=2))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_activate_parent_config(api, session, label, service, status):
    label_id = label["id"]
    service_id = service["id"]
    r = session.post(api["label"][service_id]["change_parent_label"], json=dict(parent_label_id=label_id))
    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    # make new configs
    config_data = deepcopy(old_config["data"])
    proxy_url_re = old_config["data"]["PROXY_URL_RE"]
    del config_data["PROXY_URL_RE"]
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # try activate child config but fail
    r = session.put(api["label"][service_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400

    # create new parent config
    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_p_config = r.json()["items"][0]

    config_data = deepcopy(old_p_config["data"])
    config_data["PROXY_URL_RE"] = proxy_url_re
    r = session.post(api["label"][label_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_p_config["id"]))
    assert r.status_code == 201
    p_config = r.json()
    # activating parent config
    r = session.put(api["label"][label_id]["config"][p_config["id"]][status], json=dict(old_id=old_p_config["id"]))
    assert r.status_code == 200

    # activate child config
    r = session.put(api["label"][service_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_hierarchical_configs(api, session, status):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="internal"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="external"))
    assert r.status_code == 201

    r = session.post(api['service'], json=dict(service_id="mail.yandex.ru",
                                               parent_label_id="internal",
                                               name="mail.yandex.ru",
                                               domain="mail.yandex.ru"))
    assert r.status_code == 201

    r = session.post(api['service'], json=dict(service_id="test.local",
                                               parent_label_id="external",
                                               name="test.local",
                                               domain="test.local"))
    assert r.status_code == 201

    # enabled cache in ROOT and use CRYPT_URL_PREFFIX
    r = session.get(api["label"]["ROOT"]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['USE_CACHE'] = True
    config_data['CRYPT_URL_PREFFIX'] = '/_crpd/'
    r = session.post(api["label"]["ROOT"]["config"], json=dict(data=config_data, data_settings={}, comment="ROOT Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["ROOT"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # enabled INTERNAL in internal
    r = session.get(api["label"]["internal"]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['INTERNAL'] = True
    r = session.post(api["label"]["internal"]["config"], json=dict(data=config_data, data_settings={}, comment="internal Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["internal"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # unset CRYPT_URL_PREFFIX in external
    r = session.get(api["label"]["external"]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    data_settings = {'CRYPT_URL_PREFFIX': {'UNSET': True}}
    r = session.post(api["label"]["external"]["config"], json=dict(data=config_data, data_settings=data_settings, comment="external Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["external"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # get last test.local config and check
    r = session.get(api["label"]["test.local"]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    r = session.get(api["config"][config["id"]], params=dict(status=status))
    config = r.json()
    assert r.status_code == 200
    assert config["parent_data"]["USE_CACHE"]
    assert not config["parent_data"]["INTERNAL"]
    assert config["parent_data"]["CRYPT_URL_PREFFIX"] == '/'

    # get last mail.yandex.ru config and check
    r = session.get(api["label"]["mail.yandex.ru"]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    r = session.get(api["config"][config["id"]], params=dict(status=status))
    assert r.status_code == 200
    config = r.json()
    assert config["parent_data"]["USE_CACHE"]
    assert config["parent_data"]["INTERNAL"]
    assert config["parent_data"]["CRYPT_URL_PREFFIX"] == '/_crpd/'


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("active", "test"))
def test_unset_not_exists_field(api, session, status):
    r = session.post(api['service'], json=dict(service_id="test.local",
                                               parent_label_id="ROOT",
                                               name="test.local",
                                               domain="test.local"))
    assert r.status_code == 201
    # unset field CRYPT_URL_PREFFIX
    r = session.get(api["label"]["test.local"]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    data_settings = {'CRYPT_URL_PREFFIX': {'UNSET': True}}
    r = session.post(api["label"]["test.local"]["config"], json=dict(data=config_data, data_settings=data_settings, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    # activate config
    config = r.json()
    r = session.put(api["label"]["test.local"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('params',
                         [{}, {'support_priority': 'illegal_priority'}, {'support_priority': 123}])
def test_service_change_support_priority_fail(api, session, service, params):
    service_id = service["id"]

    r = session.patch(api["service"][service_id]["support_priority"], json=params)
    assert r.status_code == 400
    assert r.json()['message'] == u'Ошибка валидации'


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('support_priority',
                         ['major', 'critical', 'minor', 'other'])
def test_service_change_support_priority_success(api, session, service, support_priority):
    service_id = service["id"]
    r = session.patch(api["service"][service_id]["support_priority"], json={'support_priority': support_priority})
    assert r.status_code == 200

    changed_service = session.get(api["service"][service_id])
    assert_that(changed_service.json(), has_entry("support_priority", support_priority))
