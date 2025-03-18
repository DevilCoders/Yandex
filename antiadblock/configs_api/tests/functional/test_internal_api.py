from copy import deepcopy

import pytest
from hamcrest import has_entry, has_entries, is_, assert_that, contains, contains_inanyorder, only_contains, has_item, is_not, equal_to

from conftest import CryproxClientMock, AUTOREDIRECT_SERVICES_DEFAULT_CONFIG


VALID_CONFIG_DATA = has_entries(PARTNER_TOKENS=only_contains(is_(basestring)),
                                ACCEL_REDIRECT_URL_RE=is_(list),
                                CLIENT_REDIRECT_URL_RE=is_(list),
                                CM_TYPE=only_contains(is_(int)),
                                CRYPT_BODY_RE=is_(list),
                                REPLACE_BODY_RE=is_(dict),
                                CRYPT_SECRET_KEY=is_(basestring),
                                CRYPT_URL_PREFFIX=is_(basestring),
                                CRYPT_URL_RE=is_(list),
                                PROXY_URL_RE=is_(list),
                                EXCLUDE_COOKIE_FORWARD=is_(list),
                                FOLLOW_REDIRECT_URL_RE=is_(list),
                                INTERNAL=is_(bool))


# test deprecate handler
@pytest.mark.usefixtures("transactional")
def test_global_get_configs_status_active_new(api, internal_api, session, service, tickets):
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
    r = session.get(api["label"][another_service["id"]]["config"]["active"])
    assert r.status_code == 200
    another_old_config = r.json()

    r = session.get(internal_api["configs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: contains(has_entries(version=old_config["id"],
                                            statuses=contains_inanyorder("test", "active", "approved"),
                                            config=is_(dict))),
        another_service["id"]: contains(has_entries(version=another_old_config["id"],
                                                    statuses=contains_inanyorder("test", "active", "approved"),
                                                    config=is_(dict))),
    }))


# test deprecate handler
@pytest.mark.usefixtures("transactional")
def test_global_get_configs_status_test_new(api, internal_api, session, service, tickets):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=old_config["data"], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"],
                    json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # create another service
    r = session.post(api['service'], json=dict(service_id="service.ru",
                                               name="another-well-known-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service = r.json()

    r = session.get(api["label"][another_service["id"]]["config"]["active"])
    assert r.status_code == 200
    another_old_config = r.json()

    r = session.post(api["label"][another_service["id"]]["config"],
                     json=dict(data=another_old_config["data"], data_settings={}, comment="Config 2", parent_id=another_old_config["id"]))
    assert r.status_code == 201
    another_config = r.json()

    r = session.put(api["label"][another_service["id"]]["config"][another_config["id"]]["test"],
                    json=dict(old_id=another_old_config["id"]))
    assert r.status_code == 200

    r = session.get(internal_api["configs"], params=dict(status="test"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: contains(has_entries(version=config["id"],
                                            statuses=contains_inanyorder("test"),
                                            config=VALID_CONFIG_DATA)),
        another_service["id"]: contains(has_entries(version=another_config["id"],
                                                    statuses=contains_inanyorder("test"),
                                                    config=VALID_CONFIG_DATA)),
    }))

    r = session.get(internal_api["configs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: contains_inanyorder(has_entries(version=config["id"],
                                                       statuses=contains_inanyorder("test"),
                                                       config=VALID_CONFIG_DATA),
                                           has_entries(version=old_config["id"],
                                                       statuses=contains_inanyorder("active", "approved"),
                                                       config=VALID_CONFIG_DATA)),
        another_service["id"]: contains_inanyorder(has_entries(version=another_config["id"],
                                                               statuses=contains_inanyorder("test"),
                                                               config=VALID_CONFIG_DATA),
                                                   has_entries(version=another_old_config["id"],
                                                               statuses=contains_inanyorder("active", "approved"),
                                                               config=VALID_CONFIG_DATA)),
    }))


# test deprecate handler
@pytest.mark.usefixtures("transactional")
def test_global_switch_service_status(api, internal_api, session, service, tickets):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]

    r = session.get(internal_api["configs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: contains(has_entries(version=config["id"],
                                            statuses=contains_inanyorder("test", "active", "approved"),
                                            config=is_(dict))),
    }))

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.get(internal_api["configs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json() == {}

    r = session.post(api["service"][service_id]["enable"])
    assert r.status_code == 200

    r = session.get(internal_api["configs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: contains(has_entries(version=config["id"],
                                            statuses=contains_inanyorder("test", "active", "approved"),
                                            config=is_(dict))),
    }))


# test all handler
@pytest.mark.usefixtures("transactional", "service")
@pytest.mark.parametrize("handler", ("configs", "configs_handler"))
def test_get_configs_no_ticket_403(internal_api, session, handler):
    r = session.get(internal_api[handler], params=dict(status="test"))
    assert r.status_code == 403


# test all handler
@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("handler", ("configs", "configs_handler"))
def test_get_services_id_with_monitorings_enabled(api, internal_api, session, service, tickets, handler):
    # create another service
    r = session.post(api['service'], json=dict(service_id="service.ru",
                                               name="another-well-known-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service = r.json()

    # change monitorings_enabled param
    r = session.post(api["service"][another_service["id"]]["set"],
                     json=dict(monitorings_enabled=False))
    assert r.status_code == 200

    r = session.get(internal_api[handler], params=dict(status="active", monitorings_enabled="true"),
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200

    services_id = list(r.json().keys())
    assert_that(services_id, has_item(service["id"]))
    assert_that(services_id, is_not(has_item(another_service["id"])))

    r = session.get(internal_api[handler], params=dict(status="active", monitorings_enabled="false"),
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200

    services_id = list(r.json().keys())
    assert_that(services_id, is_not(has_item(service["id"])))
    assert_that(services_id, has_item(another_service["id"]))


@pytest.mark.usefixtures("transactional")
def test_get_monitoring_services(api, internal_api, session, tickets):
    service_ids = ["service1.ru", "service2.ru", "service3.ru"]
    # create services
    for service_id in service_ids:
        r = session.post(api['service'], json=dict(service_id=service_id,
                                                   name=service_id,
                                                   domain=service_id))
        assert r.status_code == 201

    r = session.get(internal_api['monitoring_settings'],
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    result = r.json()
    for service_id in service_ids:
        assert_that(result[service_id], contains('desktop', 'mobile'))

    # disable all monitoring on service2.ru
    r = session.post(api["service"][service_ids[1]]["set"],
                     json=dict(monitorings_enabled=False))
    assert r.status_code == 200
    # disable monitoring for mobile on service3.ru
    r = session.post(api["service"][service_ids[2]]["set"],
                     json=dict(mobile_monitorings_enabled=False))
    assert r.status_code == 200

    r = session.get(internal_api['monitoring_settings'],
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})

    assert r.status_code == 200
    result = r.json()
    assert_that(result[service_ids[0]], contains('desktop', 'mobile'))
    assert_that(result, is_not(has_item(service_ids[1])))
    assert_that(result[service_ids[2]], contains('desktop'))


# test new handler
@pytest.mark.usefixtures("transactional", "service")
def test_get_configs_no_status_403(internal_api, session, tickets):
    r = session.get(internal_api["configs_handler"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 400


# test new handler
@pytest.mark.usefixtures("transactional", "service")
def test_get_configs_bad_status_403(internal_api, session, tickets):
    r = session.get(internal_api["configs_handler"], params=dict(status="approved"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 400


# test new handler
@pytest.mark.usefixtures("transactional")
def test_get_configs_status_active(api, internal_api, session, service, tickets):
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
    r = session.get(api["label"][another_service["id"]]["config"]["active"])
    assert r.status_code == 200
    another_old_config = r.json()

    r = session.get(internal_api["configs_handler"], params=dict(status="active"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: has_entries(version=old_config["id"],
                                   statuses=has_item("active"),
                                   config=is_(dict)),
        another_service["id"]: has_entries(version=another_old_config["id"],
                                           statuses=has_item("active"),
                                           config=is_(dict)),
    }))


# test new handler
@pytest.mark.usefixtures("transactional")
def test_get_configs_status_test(api, internal_api, session, service, tickets):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=old_config["data"], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["test"],
                    json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # create another service
    r = session.post(api['service'], json=dict(service_id="service.ru",
                                               name="another-well-known-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service = r.json()

    r = session.get(api["label"][another_service["id"]]["config"]["active"])
    assert r.status_code == 200
    another_old_config = r.json()

    r = session.post(api["label"][another_service["id"]]["config"],
                     json=dict(data=another_old_config["data"], data_settings={}, comment="Config 2", parent_id=another_old_config["id"]))
    assert r.status_code == 201
    another_config = r.json()

    r = session.put(api["label"][another_service["id"]]["config"][another_config["id"]]["test"],
                    json=dict(old_id=another_old_config["id"]))
    assert r.status_code == 200

    r = session.get(internal_api["configs_handler"], params=dict(status="test"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: has_entries(version=config["id"],
                                   statuses=has_item("test"),
                                   config=VALID_CONFIG_DATA),
        another_service["id"]: has_entries(version=another_config["id"],
                                           statuses=has_item("test"),
                                           config=VALID_CONFIG_DATA),
    }))


# test new handler
@pytest.mark.usefixtures("transactional")
def test_switch_service_status(api, internal_api, session, service, tickets):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]

    r = session.get(internal_api["configs_handler"], params=dict(status="active"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: has_entries(version=config["id"],
                                   statuses=has_item("active"),
                                   config=is_(dict)),
    }))

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.get(internal_api["configs_handler"], params=dict(status="active"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json() == {}

    r = session.post(api["service"][service_id]["enable"])
    assert r.status_code == 200

    r = session.get(internal_api["configs_handler"], params=dict(status="active"), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries({
        service["id"]: has_entries(version=config["id"],
                                   statuses=has_item("active"),
                                   config=is_(dict)),
    }))


@pytest.mark.usefixtures("transactional")
def test_decrypt_urls(internal_api, session, service, tickets):
    response = session.post(internal_api['decrypt_urls'][service['id']], json={'urls': CryproxClientMock.crypted_links.keys()},
                            headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    decrypted_urls = response.json()['urls']
    assert decrypted_urls
    assert decrypted_urls == CryproxClientMock.crypted_links.values()


@pytest.mark.usefixtures("transactional")
def test_too_much_urls_for_decrypt(internal_api, session, service, tickets):
    response = session.post(internal_api['decrypt_urls'][service['id']], json={'urls': [CryproxClientMock.crypted_links.keys()[0]] * 1001},
                            headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert response.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_decrypt_not_valid_url(internal_api, session, service, tickets):
    response = session.post(internal_api['decrypt_urls'][service['id']],
                            json={'urls': CryproxClientMock.crypted_links.keys() + ['/i/am/not/valid/url']},
                            headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    decrypted_urls = response.json()['urls']
    assert decrypted_urls
    assert decrypted_urls == CryproxClientMock.crypted_links.values() + [None]


@pytest.mark.usefixtures("transactional")
def test_service_with_webmaster_data(internal_api, session, webmaster_service_data, tickets):
    response = session.get(internal_api["configs_handler"], params=dict(status="active"),
                           headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert response.status_code == 200

    assert response.json()['autoredirect.turbo']['config']['WEBMASTER_DATA'] == AUTOREDIRECT_SERVICES_DEFAULT_CONFIG


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("handler,key", [
    ("configs_handler", "autoredirect.turbo"),
    ("configs_hierarchical_handler", "autoredirect.turbo::active::None::None"),
])
def test_add_webmaster_data(internal_api, session, webmaster_service_data, tickets, handler, key):
    response = session.get(internal_api[handler], params=dict(status="active"),
                           headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert response.status_code == 200
    assert response.json()[key]['config']['WEBMASTER_DATA'] == AUTOREDIRECT_SERVICES_DEFAULT_CONFIG

    json = {
        'aabturbo-gq': {'domain': 'aabturbo-new.gq', 'urls': ['http://aabturbo-new.gq/wp-includes/js/wp-embed.min.js']},
        'aabturbo3-gq': {'domain': 'aabturbo3.gq', 'urls': ['http://aabturbo3.gq/wp-includes/js/wp-embed.min.js']},
    }
    response = session.post(internal_api["redirect_data"], json=json,
                            headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert response.status_code == 201

    response = session.get(internal_api["configs_handler"], params=dict(status="active"),
                           headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert response.status_code == 200
    assert response.json()['autoredirect.turbo']['config']['WEBMASTER_DATA'] == json


# new hierarchical handler
@pytest.mark.usefixtures("transactional")
def test_get_hierarchical_configs(api, internal_api, session, tickets):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="internal"))
    assert r.status_code == 201
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="external"))
    assert r.status_code == 201

    # new service mail.yandex.ru
    r = session.post(api['service'], json=dict(service_id="mail.yandex.ru",
                                               parent_label_id="internal",
                                               name="mail.yandex.ru",
                                               domain="mail.yandex.ru"))
    assert r.status_code == 201

    # create mail.yandex.ru-desktop
    r = session.post(api["label"], json=dict(parent_label_id="mail.yandex.ru", label_id="mail.yandex.ru-desktop"))
    assert r.status_code == 201
    old_config = r.json()['items'][0]
    config_data = deepcopy(old_config["data"])
    config_data['DEVICE_TYPE'] = 0
    r = session.post(api["label"]["mail.yandex.ru-desktop"]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["mail.yandex.ru-desktop"]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200
    r = session.put(api["label"]["mail.yandex.ru-desktop"]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # create mail.yandex.ru-mobile
    r = session.post(api["label"], json=dict(parent_label_id="mail.yandex.ru", label_id="mail.yandex.ru-mobile"))
    assert r.status_code == 201
    old_config = r.json()['items'][0]
    config_data = deepcopy(old_config["data"])
    config_data['DEVICE_TYPE'] = 1
    r = session.post(api["label"]["mail.yandex.ru-mobile"]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["mail.yandex.ru-mobile"]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200
    r = session.put(api["label"]["mail.yandex.ru-mobile"]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # new service auto.ru
    r = session.post(api['service'], json=dict(service_id="auto.ru",
                                               parent_label_id="internal",
                                               name="auto.ru",
                                               domain="auto.ru"))
    assert r.status_code == 201
    # make experiment config (change CRYPT_URL_PREFFIX)
    r = session.get(api["label"]["auto.ru"]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    config_data = deepcopy(config["data"])
    config_data['CRYPT_URL_PREFFIX'] = '/_new_crpd/'
    r = session.post(api["label"]["auto.ru"]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.patch(api["config"][config["id"]]["experiment"], json=dict(exp_id="exp123"))
    assert r.status_code == 200

    # new service test.local
    r = session.post(api['service'], json=dict(service_id="test.local",
                                               parent_label_id="external",
                                               name="test.local",
                                               domain="test.local"))
    assert r.status_code == 201

    # create test.local-desktop
    r = session.post(api["label"], json=dict(parent_label_id="test.local", label_id="test.local-desktop"))
    assert r.status_code == 201
    old_config = r.json()['items'][0]
    config_data = deepcopy(old_config["data"])
    config_data['DEVICE_TYPE'] = 0
    r = session.post(api["label"]["test.local-desktop"]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["test.local-desktop"]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200
    r = session.put(api["label"]["test.local-desktop"]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # enabled USE_CACHE and add CRYPT_URL_PREFFIX in ROOT
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
    r = session.put(api["label"]["ROOT"]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # unset USE_CACHE in external
    r = session.get(api["label"]["external"]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    data_settings = {'USE_CACHE': {'UNSET': True}}
    r = session.post(api["label"]["external"]["config"], json=dict(data=config_data, data_settings=data_settings, comment="external Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["external"]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(internal_api["configs_hierarchical_handler"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200

    configs_data = r.json()

    assert_that(configs_data, has_entries({
        'mail.yandex.ru::active::None::None': has_entries(version=is_(int),
                                                          config=VALID_CONFIG_DATA),
        'mail.yandex.ru::test::None::None': has_entries(version=is_(int),
                                                        config=VALID_CONFIG_DATA),
        'mail.yandex.ru::active::desktop::None': has_entries(version=is_(int),
                                                             config=VALID_CONFIG_DATA),
        'mail.yandex.ru::test::desktop::None': has_entries(version=is_(int),
                                                           config=VALID_CONFIG_DATA),
        'mail.yandex.ru::active::mobile::None': has_entries(version=is_(int),
                                                            config=VALID_CONFIG_DATA),
        'mail.yandex.ru::test::mobile::None': has_entries(version=is_(int),
                                                          config=VALID_CONFIG_DATA),
        'auto.ru::active::None::None': has_entries(version=is_(int),
                                                   config=VALID_CONFIG_DATA),
        'auto.ru::test::None::None': has_entries(version=is_(int),
                                                 config=VALID_CONFIG_DATA),
        'auto.ru::None::None::exp123': has_entries(version=is_(int),
                                                   config=VALID_CONFIG_DATA),
        'test.local::active::None::None': has_entries(version=is_(int),
                                                      config=VALID_CONFIG_DATA),
        'test.local::active::desktop::None': has_entries(version=is_(int),
                                                         config=VALID_CONFIG_DATA),
        'test.local::test::desktop::None': has_entries(version=is_(int),
                                                       config=VALID_CONFIG_DATA),
        'test.local::test::None::None': has_entries(version=is_(int),
                                                    config=VALID_CONFIG_DATA),
    }))
    assert len(configs_data) == 13
    # check data in configs
    for key in ('mail.yandex.ru::active::desktop::None', 'mail.yandex.ru::active::mobile::None', 'mail.yandex.ru::active::None::None',
                'auto.ru::active::None::None', 'auto.ru::None::None::exp123'):
        assert configs_data[key]["config"]["USE_CACHE"]

    for key in ('test.local::active::desktop::None', 'test.local::active::None::None',
                'mail.yandex.ru::test::desktop::None', 'mail.yandex.ru::test::mobile::None',  'mail.yandex.ru::test::None::None',
                'auto.ru::test::None::None', 'test.local::test::desktop::None'):
        assert not configs_data[key]["config"]["USE_CACHE"]

    for key in ('mail.yandex.ru::active::desktop::None', 'mail.yandex.ru::active::mobile::None', 'mail.yandex.ru::active::None::None',
                'auto.ru::active::None::None', 'test.local::active::desktop::None'):
        assert configs_data[key]["config"]["CRYPT_URL_PREFFIX"] == "/_crpd/"

    for key in ('auto.ru::None::None::exp123', ):
        assert configs_data[key]["config"]["CRYPT_URL_PREFFIX"] == "/_new_crpd/"

    for key in ('mail.yandex.ru::test::desktop::None', 'mail.yandex.ru::test::mobile::None', 'mail.yandex.ru::test::None::None',
                'auto.ru::test::None::None', 'test.local::test::desktop::None', 'test.local::test::None::None'):
        assert configs_data[key]["config"]["CRYPT_URL_PREFFIX"] == "/"


@pytest.mark.usefixtures("transactional", "service")
def test_change_current_cookie_status_403(internal_api, session, tickets):
    r = session.post(internal_api["change_current_cookie"])
    assert r.status_code == 403


@pytest.mark.usefixtures("transactional", "service")
def test_change_current_cookie_bad_request(internal_api, session, tickets):
    r = session.post(internal_api["change_current_cookie"],
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 400


@pytest.mark.usefixtures("transactional", "service")
def test_change_current_cookie_bad_service(internal_api, session, tickets):
    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': 'bad_service'},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 404


@pytest.mark.usefixtures("transactional", "service")
def test_change_current_cookie_no_cookie_day(internal_api, session, tickets):
    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': 'auto.ru'},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()['message'] == 'No cookie of the day on service'


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_change_current_cookie_no_rules(api, internal_api, session, service, tickets, status):
    service_id = service['id']
    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['CURRENT_COOKIE'] = 'newcookie'
    config_data['WHITELIST_COOKIES'] = ['newcookie']
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["ROOT"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': 'auto.ru',
                           'test': True if status == 'test' else False},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})

    assert r.status_code == 400
    assert r.json()['message'] == 'No cookie remover rules'


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_change_current_cookie_not_matched(api, internal_api, session, tickets, status):
    # new service test.service
    service_id = 'test.service'
    r = session.post(api['service'], json=dict(service_id=service_id,
                                               parent_label_id="ROOT",
                                               name="test.local",
                                               domain="test.local"))

    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['CURRENT_COOKIE'] = 'newcookie'
    config_data['WHITELIST_COOKIES'] = ['newcookie']
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["ROOT"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': service_id,
                           'test': True if status == 'test' else False},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), equal_to({'message': 'Current cookie not matched for actual rules', 'current_cookie': 'newcookie'}))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_change_current_cookie_all_cookies_matched(api, internal_api, session, tickets, status):
    # new service test.service
    service_id = 'test.service'
    r = session.post(api['service'], json=dict(service_id=service_id,
                                               parent_label_id="ROOT",
                                               name="test.local",
                                               domain="test.local"))

    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['CURRENT_COOKIE'] = 'specific'
    config_data['WHITELIST_COOKIES'] = ['specific', 'substantial']
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["ROOT"]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': service_id,
                           'test': True if status == 'test' else False},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 400
    assert r.json()['message'] == 'All cookies matched for actual rules'


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_change_current_cookie_update_cookie_current_label(api, internal_api, session, tickets, status):
    # new service test.service
    service_id = 'test.service'
    r = session.post(api['service'], json=dict(service_id=service_id,
                                               parent_label_id="ROOT",
                                               name="test.local",
                                               domain="test.local"))

    assert r.status_code == 201

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['CURRENT_COOKIE'] = 'specific'
    config_data['WHITELIST_COOKIES'] = ['specific', 'newcookie', 'somecookie']
    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"][service_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': service_id,
                           'test': True if status == 'test' else False},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201

    assert_that(r.json(), equal_to({"new_cookie": 'newcookie', "available_cookies_count": 1, "label_id": service_id}))

    r = session.get(api["label"][service_id]["config"][status])
    assert r.status_code == 200
    data = r.json()['data']
    assert data['CURRENT_COOKIE'] == 'newcookie'
    assert data['DEPRECATED_COOKIES'] == ['specific']


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_change_current_cookie_update_cookie_parent_label(api, internal_api, session, tickets, status):
    label_id = 'test.parent'
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id=label_id))
    assert r.status_code == 201
    # new service test.service
    service_id = 'test.service'
    r = session.post(api['service'], json=dict(service_id=service_id,
                                               parent_label_id=label_id,
                                               name="test.local",
                                               domain="test.local"))

    assert r.status_code == 201

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['CURRENT_COOKIE'] = 'specific'
    config_data['WHITELIST_COOKIES'] = ['specific', 'newcookie']
    r = session.post(api["label"][label_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"][label_id]["config"][config["id"]][status], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200
    old_config_id = config["id"]

    r = session.post(internal_api["change_current_cookie"],
                     json={'service_id': service_id,
                           'test': True if status == 'test' else False},
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201
    assert_that(r.json(), equal_to({"new_cookie": 'newcookie', "available_cookies_count": 0, "label_id": label_id}))

    r = session.get(api["label"][label_id]["config"][status])
    assert r.status_code == 200
    config_id = r.json()["id"]
    data = r.json()['data']
    assert data['CURRENT_COOKIE'] == 'newcookie'
    assert data['DEPRECATED_COOKIES'] == ['specific']

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=1), headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()['items'][0]['user_name'] == "Mr. Robot"
    assert_that(r.json()['items'][0]['params'],
                equal_to({'config_comment': 'New cookie newcookie', 'new_cookie': 'newcookie', 'config_id': config_id, 'old_config_id': old_config_id}))


@pytest.mark.usefixtures("transactional")
def test_get_services_balancer_settings(api, internal_api, session, service, tickets):
    service_id = service["id"]

    config_prod = dict(
        CRYPT_ENABLE_TRAILING_SLASH=True,
        CRYPT_URL_PREFFIX='a',
        CRYPT_URL_OLD_PREFFIXES=['b.', 'c'],
        CRYPT_SECRET_KEY='secret_prod',
        PARTNER_BACKEND_URL_RE=['prod', '1'],
        BALANCERS_PROD=['balancer_prod']
    )
    expected_prod = dict(
        crypt_enable_trailing_slash=True,
        crypt_preffixes=r'a|b\.|c',
        crypt_secret_key='secret_prod',
        backend_url_re='prod|1',
        service_id=service_id,
    )

    config_test = dict(
        CRYPT_ENABLE_TRAILING_SLASH=False,
        CRYPT_URL_PREFFIX='1',
        CRYPT_SECRET_KEY='secret_test',
        PARTNER_BACKEND_URL_RE=[r'test\/.+', '1'],
        BALANCERS_TEST=['balancer_test']
    )
    expected_test = dict(
        crypt_enable_trailing_slash=False,
        crypt_preffixes='1',
        crypt_secret_key='secret_test',
        backend_url_re=r'test\/.+|1',
        service_id=service_id,
    )

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    # Creates config from settings_prod and activates it for prod
    config = deepcopy(old_config["data"])
    config.update(config_prod)
    r = session.post(api["label"][service_id]["config"], json=dict(data=config, data_settings={}, comment="Config Prod", parent_id=old_config["id"]))
    assert r.status_code == 201
    config_prod_id = r.json()['id']
    r = session.put(api["label"][service_id]["config"][config_prod_id]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # Only prod balancers set
    r = session.get(internal_api['services']['balancer_settings'], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entry(service_id, has_entries(balancer_prod=expected_prod)))

    # Creates config from settings_test and activates it for test
    config = deepcopy(old_config["data"])
    config.update(config_test)
    r = session.post(api["label"][service_id]["config"], json=dict(data=config, data_settings={}, comment="Config Test", parent_id=old_config["id"]))
    assert r.status_code == 201
    config_test_id = r.json()['id']
    r = session.put(api["label"][service_id]["config"][config_test_id]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # Prod and test balancers set
    r = session.get(internal_api['services']['balancer_settings'], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entry(service_id, has_entries(balancer_prod=expected_prod, balancer_test=expected_test)))
