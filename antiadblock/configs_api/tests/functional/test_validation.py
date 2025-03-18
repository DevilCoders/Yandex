# coding=utf-8

from copy import deepcopy

import pytest
from hamcrest import has_entries, contains, assert_that, has_item

from antiadblock.configs_api.lib.validation.template import CookieMatching
from conftest import Session, USER2_LOGIN, USER2_SESSION_ID


@pytest.mark.parametrize('initial_prefixes, expected_code', [
    (["/prefix_c/", "/prefix_a/", "/prefix_b/", "/prefix_a/"], 400),
    (["/prefix_a/", "/prefix_b/", "/prefix_c/"], 201),
])
@pytest.mark.usefixtures("transactional")
def test_valid_prefixes_list(api, session, service, initial_prefixes, expected_code):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1

    config = r.json()["items"][0]
    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RANDOM_PREFFIXES"] = initial_prefixes
    config_data["CRYPT_URL_OLD_PREFFIXES"] = initial_prefixes

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == expected_code
    details = r.json()
    # assert_that(details, equal_to(None))
    if expected_code != 201:
        assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "CRYPT_URL_RANDOM_PREFFIXES")),
                                                             has_entries(path=contains("data", "CRYPT_URL_OLD_PREFFIXES")))))


@pytest.mark.usefixtures("transactional")
def test_remove_tokens(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["PARTNER_TOKENS"] = []

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS")))))


@pytest.mark.usefixtures("transactional")
def test_wrong_token(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["PARTNER_TOKENS"] = ["aaaavbbbbbbcadfsaasdfxxvzsfasdfasdfsa"]

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS", 0)))))


@pytest.mark.usefixtures("transactional")
def test_valid_and_wrong_token(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["PARTNER_TOKENS"].append("aaaavbbbbbbcadfsaasdfxxvzsfasdfasdfsa")

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS", 1)))))


@pytest.mark.usefixtures("transactional")
def test_2_wrong_tokens(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["PARTNER_TOKENS"] = ["aaaavbbbbbbcadfsaasdfxxvzsfasdfasdfsa", "123123123123123123"]

    r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS", 0)),
                                                         has_entries(path=contains("data", "PARTNER_TOKENS", 1)))))


@pytest.mark.usefixtures("transactional")
def test_exchange_tokens_on_activate(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # exchange on valid token
    # generate 08.02.2019, valid 3650 days
    config_data["PARTNER_TOKENS"] = [
        "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1NDk2MTcxMzAsInN1YiI6ImF1dG8ucnUiLCJleHAiOjE4NjQ5ODc5MzB9.oBKgR2Oeb2TrBhfVkM7YGigchlxvHYEaSsueKvkDLjCdKUgy_aBLYnI3b4FdQobEi"
        + "LjP68lqaKkw1niyMfbB0IyEEvUNyS4-f0B1_oc4Q1cLYe-Gibsw23q217DHTOBym9PodkHrSNFna3q4iq-0oEVAwia55bm7hPHMBSI3gu-UNCHKhCWGth6L2c_Z6IOiEoCoB8uHCfawlk2OtORDcbOVUlqGkRAzQDAXZqqT8zU8IZfnlfFuIejCw0"
        + "m-FIxeQDNbeQEnkCt9Y-cddGgCC1fCqEimzHQp471O94DUs3bQrJamv6F5TYHn6bytXZnQfDfhAqD1W3Y-LESTey9ltA"]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.patch(api["label"][service_id]["config"][config["id"]]["moderate"],
                      json=dict(approved=True, comment="Approved"))
    assert r.status_code == 200

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS")))))


@pytest.mark.skip(reason="Don't check expired token")
@pytest.mark.usefixtures("transactional")
def test_put_only_expired_tokens(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # exchange on expired token
    config_data["PARTNER_TOKENS"] = [
        "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTc1ODcwODIsInN1YiI6ImF1dG8ucnUiLCJleHAiOjE1MTc1ODcwODJ9.ZX3GL-SFDFQklYnQdy7rSBPk55HA1V9xA3QWiv2RDda3mrqsxrfQ7RRUgqws7Foo"
        + "D1ZLnTbKP8lQdYvfpoyVsU3BC9T2c2uuSEHvOiOZUDVbZzEBlIH7FLuU64L31rZIlnSv1CgVya5VUcS0vktcYIM1xtvAuj02ulZE3FCbvshC01aC_SLhiP_4Kqt-LcfYxz6HJa9OQ1D76S1E7s3QpQ3j6lfA2g28l19lcNrsf8RFAyfENBZOtIbW"
        + "fnShlFtRa7MX1fZJFpm1stYjy20q2fct7iwGi9oyCwROIgodhapl1pPmov0dASAeo7UVIHy7Jm437F59ViuDUqvPV1Jmnw"]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS")))))


@pytest.mark.usefixtures("transactional")
def test_put_different_partner_token(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # token for ne.auto.ru
    # generate 08.02.2019, valid 3650 days
    config_data["PARTNER_TOKENS"] = [
        "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1NDk2MTc0NTEsInN1YiI6Im5lLmF1dG8ucnUiLCJleHAiOjE4NjQ5ODgyNTF9.o2yNHVhDBv2oSYUn5ZOQvP-CRwppsdy2"
        + "mGp_5taSp5KLwPPP7deN8E33Ym76br6x4YhTucdthxvw0aUVHmGClTnLIsDxYD4rYm1uDjFqzq2C-zblkioZiSFcGkWZw7l6fOP9k9YRpgWep45vXfEvYterIKqm7R-WUY2ba_gIeWfib1UbQ7OValkxjLwT"
        + "Awwfze4K1304NMW5GwZjqyiu_5D0Kcsl4QCJbrlJphZkQbpzjmlxweHgC1HTAh4us6PKUcyivcQ_QpYUTXF0UkR2pGZLO3BVLIcFfxL9OHgqPV3AjYvJhLDmcUAapn5OfnrVHLXRj7X8HP6bWRO9z-aQRw"
    ]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS", 0)))))


@pytest.mark.usefixtures("transactional")
def test_put_different_partner_fixed_token(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # token for ne.auto.ru
    config_data["PARTNER_TOKENS"] = [
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MDQ2MTAxODksInN1YiI6InR2X3Byb2dyYW1tYSIsImV4cCI6MTUyMDM0NTc4OX0.73ZL3_lDGZ09VyvB4x-KJnNttbH4vM9rCciCrMu-VUI"
    ]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS", 0)))))


@pytest.mark.usefixtures("transactional")
def test_put_invalid_token(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # token for ne.auto.ru
    config_data["PARTNER_TOKENS"] = [1]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PARTNER_TOKENS", 0)))))


@pytest.mark.usefixtures("transactional")
def test_all_cm_types(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # token for ne.auto.ru
    config_data["CM_TYPE"] = [CookieMatching.CRYPTED_UID, CookieMatching.IMAGE, CookieMatching.REFRESH]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201

    config_id = r.json()["id"]
    r = session.get(api["config"][config_id])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(data=has_entries(CM_TYPE=contains(CookieMatching.CRYPTED_UID, CookieMatching.IMAGE, CookieMatching.REFRESH))))


@pytest.mark.usefixtures("transactional")
def test_wrong_cm_order(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    # token for ne.auto.ru
    config_data["CM_TYPE"] = [CookieMatching.REFRESH, CookieMatching.IMAGE, CookieMatching.CRYPTED_UID]

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "CM_TYPE")))))


@pytest.mark.usefixtures("transactional")
def test_missing_required_field_in_config(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    del config_data["PROXY_URL_RE"]
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "PROXY_URL_RE")))))


@pytest.mark.usefixtures("transactional")
def test_send_config_no_changes(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    assert_that(r.json(), has_entries(data=config_data))


@pytest.mark.usefixtures("transactional")
def test_static_test_passes(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["TEST_DATA"] = dict(valid_domains=["auto.ru"])
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    assert_that(r.json(), has_entries(data=config_data))


@pytest.mark.usefixtures("transactional")
def test_static_test_valid_domains_fails(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["TEST_DATA"] = dict(valid_domains=["neauto.ru"])
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "TEST_DATA", "valid_domains", 0)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_invalid_domains_fails(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["TEST_DATA"] = dict(invalid_domains=["auto.ru"])
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details, has_entries(properties=contains(has_entries(path=contains("data", "TEST_DATA", "invalid_domains", 0)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_valid_and_invalid_pass(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"], valid_domains=["auto.ru"])
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_static_test_incorrect_paths(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"], valid_domains=["auto.ru"], paths={})
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=contains(has_entries(path=contains("data", "TEST_DATA", "paths")))))


@pytest.mark.usefixtures("transactional")
def test_static_test_correct_paths(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes|bently)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    valid_domains=["auto.ru", "ua.auto.ru"],
                                    paths=dict(valid=["mercedes", "bently"],
                                               invalid=["lada"]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_static_test_invalid_paths(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes|bently)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    valid_domains=["auto.ru", "ua.auto.ru"],
                                    paths=dict(valid=["mercedes", "bently"],
                                               invalid=[1]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=has_item(has_entries(path=contains("data", "TEST_DATA", "paths", "invalid", 0)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_paths_validation_fails_on_valid(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    valid_domains=["auto.ru", "ua.auto.ru"],
                                    paths=dict(valid=["mercedes", "bently"],
                                               invalid=["lada"]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=has_item(has_entries(path=contains("data", "TEST_DATA", "paths", "valid", 1)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_paths_validation_fails_on_invalid(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes|bently)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    valid_domains=["auto.ru", "ua.auto.ru"],
                                    paths=dict(valid=["mercedes"],
                                               invalid=["lada", "bently"]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=has_item(has_entries(path=contains("data", "TEST_DATA", "paths", "invalid", 1)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_paths_validation_fails_on_invalid_partitial(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes|bently)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    valid_domains=["auto.ru", "ua.auto.ru"],
                                    paths=dict(invalid=["lada", "bently"]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=has_item(has_entries(path=contains("data", "TEST_DATA", "paths", "invalid", 1)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_paths_validation_fails_on_valid_partitial(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    valid_domains=["auto.ru", "ua.auto.ru"],
                                    paths=dict(valid=["mercedes", "bently"]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=has_item(has_entries(path=contains("data", "TEST_DATA", "paths", "valid", 1)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_2_invalid_domains(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["TEST_DATA"] = dict(valid_domains=["neauto.ru", "neauto2.ru"])
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=contains(has_entries(path=contains("data", "TEST_DATA", "valid_domains", 0)),
                                                has_entries(path=contains("data", "TEST_DATA", "valid_domains", 1)))))


@pytest.mark.usefixtures("transactional")
def test_static_test_paths_validation_without_valid_domains(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])

    config_data["PROXY_URL_RE"] = ["(?:\\w+\\.)*auto\\.ru/(?:mercedes)/.*?"]
    config_data["TEST_DATA"] = dict(invalid_domains=["neauto.ru"],
                                    paths=dict(valid=["mercedes", "bently"]))
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.get(api["label"][service_id]["config"]["active"])
    assert r.status_code == 200
    old_config = r.json()

    r = session.put(api["label"][service_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=has_item(has_entries(path=contains("data", "TEST_DATA", "valid_domains")))))


@pytest.mark.parametrize('config_key,config_value', [("ACCEL_REDIRECT_URL_RE", ["[123"]),
                                                     ("CLIENT_REDIRECT_URL_RE", ["[123"]),
                                                     ("CRYPT_RELATIVE_URL_RE", ["[123"]),
                                                     ("CRYPT_URL_RE", ["[123"]),
                                                     ("FOLLOW_REDIRECT_URL_RE", ["[123"]),
                                                     ("CRYPT_BODY_RE", ["[123"])])
@pytest.mark.usefixtures("transactional")
def test_broken_regexp(api, session, service, config_key, config_value):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data.update({config_key: config_value})

    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    path = ["data", config_key]
    if isinstance(config_value, list):
        path.append(0)
    if isinstance(config_value, dict):
        path.append(next(config_value.iterkeys()))
    assert_that(details, has_entries(properties=contains(has_entries(path=contains(*path)))))


@pytest.mark.usefixtures("transactional")
def test_user_cant_change_hidden_fields(api, service, grant_permissions):
    user_session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")

    r = user_session.get(api["label"][service["id"]]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CM_TYPE"] = [1]
    r = user_session.post(api["label"][service['id']]["config"],
                          json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == 400
    details = r.json()
    assert_that(details,
                has_entries(properties=contains(has_entries(path=contains("data", "CM_TYPE")))))


@pytest.mark.parametrize('new_url,expected_code', [
    ("", 201),
    ("fake_url", 400),
    ("https://aab-pub.s3.yandex.net/lib.browser.min.js", 201),
])
@pytest.mark.usefixtures("transactional")
def test_url_new_detect_script(api, session, service, new_url, expected_code):
    service_id = service["id"]
    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    config_data = deepcopy(config["data"])
    config_data.update({"NEW_DETECT_SCRIPT_URL": new_url})
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == expected_code


# data for experiments
VALID_EXP_1 = {
    'EXPERIMENT_TYPE': 2,
    'EXPERIMENT_PERCENT': 10,
    'EXPERIMENT_START': "2020-01-01T18:00:00",
    'EXPERIMENT_DURATION': 4,
    'EXPERIMENT_DAYS': [0, 2, 4],
    'EXPERIMENT_DEVICE': [1],
}

VALID_EXP_2 = {
    'EXPERIMENT_TYPE': 2,
    'EXPERIMENT_PERCENT': 10,
    'EXPERIMENT_START': "2020-01-01T18:00:00",
    'EXPERIMENT_DURATION': 4,
    'EXPERIMENT_DAYS': [1, 3],
    'EXPERIMENT_DEVICE': [1],
}

VALID_EXP_3 = {
    'EXPERIMENT_TYPE': 2,
    'EXPERIMENT_PERCENT': 10,
    'EXPERIMENT_START': "2020-01-01T16:00:00",
    'EXPERIMENT_DURATION': 4,
    'EXPERIMENT_DAYS': [0],
    'EXPERIMENT_DEVICE': [1],
}

INVALID_EXP_1 = {
    'EXPERIMENT_TYPE': 1,
    'EXPERIMENT_PERCENT': 10,
    'EXPERIMENT_START': "2020-01-01T16:00:00",
    'EXPERIMENT_DURATION': 4,
    'EXPERIMENT_DEVICE': [],  # bad field
}


@pytest.mark.parametrize('experiment_data,expected_code,error_msg', [
    ([], 201, ""),
    ([VALID_EXP_1], 201, ""),
    ([VALID_EXP_1, VALID_EXP_2], 201, ""),
    ([VALID_EXP_1, VALID_EXP_2, VALID_EXP_3], 400, u"Эксперименты пересекаются"),  # intersection exp_1 and exp_3
    ([INVALID_EXP_1], 400, u"Некорректные данные"),  # bad data
])
@pytest.mark.usefixtures("transactional")
def test_validate_experiment_data(api, session, service, experiment_data, expected_code, error_msg):
    service_id = service["id"]
    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    config_data = deepcopy(config["data"])
    config_data.update({"EXPERIMENTS": experiment_data})
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == expected_code
    if expected_code == 400:
        assert r.json()["message"] == u"Ошибка валидации"
        for el in r.json()["properties"]:
            assert el["message"] == error_msg


@pytest.mark.parametrize('csp_patch,expected_code,error_msg', [
    ({}, 201, ""),
    ({"connect-src": ["strm.yandex.net", "*.strm.yandex.net"]}, 201, ""),
    ({"connect-src": ["strm.yandex.net", "*.strm.yandex.net"], "media-src": ["blob:"]}, 201, ""),
    ({"connect-src": []}, 400, u"Пустые значения для политик: connect-src."),
    ({"connect-src": ["strm.yandex.net", "*.strm.yandex.net"], "bad-src": ["strm.yandex.net", "*.strm.yandex.net"]}, 400, u"Недопустимые политики: bad-src."),
])
@pytest.mark.usefixtures("transactional")
def test_validate_csp_patch(api, session, service, csp_patch, expected_code, error_msg):
    service_id = service["id"]
    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config = r.json()["items"][0]
    config_data = deepcopy(config["data"])
    config_data.update({"CSP_PATCH": csp_patch})
    r = session.post(api["label"][service_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=config["id"]))
    assert r.status_code == expected_code
    if expected_code == 400:
        assert r.json()["message"] == u"Ошибка валидации"
        for el in r.json()["properties"]:
            assert el["message"] == error_msg
