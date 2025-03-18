import pytest
from hamcrest import assert_that, has_entries, contains, contains_inanyorder, \
    only_contains, is_, any_of, anything, empty

from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from conftest import USER2_SESSION_ID, USER3_SESSION_ID, Session, USER2_LOGIN, \
    USER3_LOGIN, UNKNOWN_SESSION_ID


@pytest.mark.usefixtures("transactional")
def test_admin_has_global_permissions(api, session):
    response = session.get(api["auth"]["permissions"]["global"])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=contains_inanyorder(*PermissionKind.all())))


@pytest.mark.usefixtures("transactional")
def test_admin_has_permissions_on_service(api, session, service):
    response = session.get(api["auth"]["permissions"]["service"][service["id"]])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=contains_inanyorder(*PermissionKind.all())))


@pytest.mark.usefixtures("transactional")
def test_user_has_no_global_permissions(api, service, grant_permissions):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")
    response = session.get(api["auth"]["permissions"]["global"])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=[]))


@pytest.mark.usefixtures("transactional")
def test_guest_has_no_global_permissions(api):
    session = Session(UNKNOWN_SESSION_ID)
    response = session.get(api["auth"]["permissions"]["global"])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=[]))


@pytest.mark.usefixtures("transactional")
def test_guest_has_no_service_permissions(api, service):
    session = Session(UNKNOWN_SESSION_ID)
    response = session.get(api["auth"]["permissions"]["service"][service["id"]])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=[]))


@pytest.mark.usefixtures("transactional")
def test_user_has_permissions_on_service(api, service, grant_permissions):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")
    response = session.get(api["auth"]["permissions"]["service"][service["id"]])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=contains_inanyorder("service_see", "config_create", "config_mark_test", "config_mark_active", "config_archive")))


@pytest.mark.usefixtures("transactional")
def test_user_has_no_permissions_on_different_service(api, session, grant_permissions):
    session1 = Session(USER2_SESSION_ID)
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 201
    service1 = response.json()
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201
    service2 = response.json()
    grant_permissions(USER2_LOGIN, service1['id'], "external_user")
    response = session1.get(api["auth"]["permissions"]["service"][service2["id"]])
    assert response.status_code == 200
    assert_that(response.json(), has_entries(permissions=[]))


@pytest.mark.usefixtures("transactional")
def test_get_only_permitted_services(api, session, grant_permissions):
    session1 = Session(USER2_SESSION_ID)
    session2 = Session(USER3_SESSION_ID)
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 201
    service1 = response.json()
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201
    service2 = response.json()

    grant_permissions(USER2_LOGIN, service1['id'], "external_user")
    grant_permissions(USER3_LOGIN, service2['id'], "external_user")

    r = session1.get(api["services"])
    assert r.status_code == 200
    # should be ordered by name
    assert_that(r.json(), has_entries(items=contains(has_entries(name="morda")),
                                      total=1))

    r = session2.get(api["services"])
    assert r.status_code == 200
    # should be ordered by name
    assert_that(r.json(), has_entries(items=contains(has_entries(name="autoru")),
                                      total=1))


@pytest.mark.usefixtures("transactional")
def test_user_can_create_config(api, grant_permissions, service):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")

    r = session.get(api["label"][service['id']]["config"]['active'])
    assert r.status_code == 200
    old_config = r.json()

    r = session.post(api["label"][service['id']]["config"],
                     json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_user_can_test_config(api, grant_permissions, service):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")

    r = session.get(api["label"][service['id']]["config"]['active'])
    assert r.status_code == 200
    old_config = r.json()

    r = session.post(api["label"][service['id']]["config"],
                     json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][service['id']]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200


@pytest.mark.usefixtures("transactional")
def test_user_cant_activate_config(api, grant_permissions, service):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")

    r = session.get(api["label"][service['id']]["config"]['active'])
    assert r.status_code == 200
    old_config = r.json()

    r = session.post(api["label"][service['id']]["config"],
                     json=dict(data=old_config['data'], data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][service['id']]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 403


@pytest.mark.usefixtures("transactional")
def test_user_cant_create_service(api):
    session = Session(USER2_SESSION_ID)

    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 403


@pytest.mark.usefixtures("transactional")
def test_nonlogon_403(app, valid_app_urls):
    # If you write new control with new placeholder you should add it to substitute_map at valid_app_urls fixture

    session = Session(USER2_SESSION_ID)
    for url, method in valid_app_urls:
        args = [app[url]]
        kvargs = {}
        if method != 'get':
            kvargs['json'] = {}
        r = getattr(session, method)(*args, **kvargs)
        if r.status_code != 403:
            assert False


@pytest.mark.usefixtures("transactional")
def test_user_have_permissions_on_webmaster_domain(api, session, service_by_webmaster):
    session = Session(USER2_SESSION_ID)

    r = session.get(api["auth"]["permissions"]["service"][service_by_webmaster['id']])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(permissions=contains_inanyorder("service_see", "config_mark_test", "config_mark_active", "config_create", "config_archive")))

    r = session.get(api["services"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(total=1,
                                      items=contains(has_entries(id=service_by_webmaster["id"]))))

    r = session.get(api["label"][service_by_webmaster["id"]]["configs"])
    assert r.status_code == 200


@pytest.mark.usefixtures("transactional")
def test_user_have_permissions_on_2_domains(api, session, service, service_by_webmaster, grant_permissions):
    grant_permissions(USER2_LOGIN, service['id'], "internal_user")
    user_session = Session(USER2_SESSION_ID)

    r = user_session.get(api["auth"]["permissions"]["service"][service_by_webmaster['id']])
    assert r.status_code == 200
    assert_that(r.json(),
                has_entries(permissions=contains_inanyorder("service_see", "config_mark_test", "config_mark_active", "config_create", "config_archive")))

    r = user_session.get(api["auth"]["permissions"]["service"][service['id']])
    assert r.status_code == 200
    assert_that(r.json(),
                has_entries(permissions=contains_inanyorder("service_see", "config_mark_test", "config_mark_active", "config_create", "config_archive")))

    r = user_session.get(api["services"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(total=2,
                                      items=contains(has_entries(id=service["id"]),
                                                     has_entries(id=service_by_webmaster["id"]))))


@pytest.mark.usefixtures("transactional")
def test_user_have_permissions_admin_and_webmaster_permissions(api, grant_permissions, service_by_webmaster):
    user_session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service_by_webmaster['id'], "admin")

    r = user_session.get(api["auth"]["permissions"]["service"][service_by_webmaster['id']])
    assert r.status_code == 200
    assert_that(r.json(),
                has_entries(permissions=contains_inanyorder(*PermissionKind.all())))

    r = user_session.get(api["services"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(total=1,
                                      items=contains(has_entries(id=service_by_webmaster["id"]))))


@pytest.mark.usefixtures("transactional")
def test_global_admin_has_admin_permissions_on_wm_service(api, grant_permissions, service_by_webmaster):
    user_session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, "*", "admin")

    r = user_session.get(api["auth"]["permissions"]["service"][service_by_webmaster['id']])
    assert r.status_code == 200
    assert_that(r.json(),
                has_entries(permissions=contains_inanyorder(*PermissionKind.all())))

    r = user_session.get(api["services"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(total=1,
                                      items=contains(has_entries(id=service_by_webmaster["id"]))))


@pytest.mark.usefixtures("transactional")
def test_user_cant_see_hidden_fields(api, service, grant_permissions):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")
    r = session.get(api["label"][service["id"]]["configs"])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=only_contains(has_entries(data=has_entries(PROXY_URL_RE=is_(list))))))

    assert all([hidden_key not in r.json()['items'][0]['data'] for hidden_key in ["PARTNER_TOKENS", "PUBLISHER_SECRET_KEY", "CRYPT_SECRET_KEY", "CM_TYPE", "INTERNAL"]])


def valid_schema_field_descriptor(name):
    return any_of(has_entries(type_schema=has_entries(type=is_(basestring),
                                                      hint=any_of(is_(basestring), is_(dict)),
                                                      title=any_of(is_(basestring), is_(dict)),
                                                      placeholder=any_of(is_(basestring), is_(dict))),
                              default=anything(),
                              name=name),
                  has_entries(type_schema=has_entries(type=is_(basestring),
                                                      hint=any_of(is_(basestring), is_(dict)),
                                                      title=any_of(is_(basestring), is_(dict)),
                                                      children=has_entries(type=is_(basestring),
                                                                           placeholder=any_of(is_(basestring), is_(dict)))),
                              default=anything(),
                              name=name))


@pytest.mark.usefixtures("transactional")
def test_user_cant_switch_service_status(api, service):
    session = Session(USER2_SESSION_ID)
    response = session.post(api["service"][service["id"]]["disable"])
    assert response.status_code == 403


@pytest.mark.usefixtures("transactional")
def test_user_can_get_form_schema(api, service, grant_permissions):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")
    r = session.get(api["label"][service["id"]]["config"]["schema"])
    assert r.status_code == 200
    schema = r.json()
    assert_that(schema, only_contains(
        has_entries(group_name="SECURITY", title=is_(basestring), hint=is_(basestring),
                    items=contains(
                        has_entries(key=valid_schema_field_descriptor('PROXY_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('EXCLUDE_COOKIE_FORWARD')),
                        has_entries(key=valid_schema_field_descriptor('CURRENT_COOKIE')),
                        has_entries(key=valid_schema_field_descriptor('DEPRECATED_COOKIES')),
                        has_entries(key=valid_schema_field_descriptor('WHITELIST_COOKIES')))),
        has_entries(group_name="ENCRYPTION", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('CRYPT_BODY_RE')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_BODY_RE')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('BYPASS_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_RELATIVE_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_IN_LOWERCASE')))),
        has_entries(group_name="ENCRYPTION_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('AD_SYSTEMS')))),
        has_entries(group_name="FEATURES", title=is_(basestring), hint=is_(basestring),
                    items=empty()),
        has_entries(group_name="DETECT_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('DETECT_HTML')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_LINKS')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_TRUSTED_LINKS')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_IFRAME')),
                        has_entries(key=valid_schema_field_descriptor('DISABLE_DETECT')))),
        has_entries(group_name="COOKIEMATCHING_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=empty()),
        has_entries(group_name="INTERNAL_PARTNER_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=empty()),
        has_entries(group_name="REDIRECT_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('FOLLOW_REDIRECT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('CLIENT_REDIRECT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('ACCEL_REDIRECT_URL_RE')))),
        has_entries(group_name="DEBUG", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('TEST_DATA')))),
        has_entries(group_name="EXPERIMENTS", title=is_(basestring), hint=is_(basestring), items=empty()),
    ))


@pytest.mark.usefixtures("transactional")
def test_admin_can_get_full_form_schema(api, session, service, grant_permissions):
    r = session.get(api["label"][service["id"]]["config"]["schema"])
    assert r.status_code == 200
    schema = r.json()
    assert_that(schema, only_contains(
        has_entries(group_name="SECURITY", title=is_(basestring), hint=is_(basestring),
                    items=contains(
                        has_entries(key=valid_schema_field_descriptor('PROXY_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('EXCLUDE_COOKIE_FORWARD')),
                        has_entries(key=valid_schema_field_descriptor('CURRENT_COOKIE')),
                        has_entries(key=valid_schema_field_descriptor('DEPRECATED_COOKIES')),
                        has_entries(key=valid_schema_field_descriptor('WHITELIST_COOKIES')),
                        has_entries(key=valid_schema_field_descriptor('CRYPTED_YAUID_COOKIE_NAME')),
                        has_entries(key=valid_schema_field_descriptor('PARTNER_TOKENS')),
                        has_entries(key=valid_schema_field_descriptor('PUBLISHER_SECRET_KEY')))),
        has_entries(group_name="ENCRYPTION", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('CRYPT_BODY_RE')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_BODY_RE')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_BODY_RE_PER_URL')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_BODY_RE_EXCEPT_URL')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('BYPASS_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_RELATIVE_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_IN_LOWERCASE')))),
        has_entries(group_name="ENCRYPTION_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('DEVICE_TYPE')),
                        has_entries(key=valid_schema_field_descriptor('ENCRYPTION_STEPS')),
                        has_entries(key=valid_schema_field_descriptor('SEED_CHANGE_PERIOD')),
                        has_entries(key=valid_schema_field_descriptor('SEED_CHANGE_TIME_SHIFT_MINUTES')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_SECRET_KEY')),
                        has_entries(key=valid_schema_field_descriptor('COOKIE_CRYPT_KEY')),
                        has_entries(key=valid_schema_field_descriptor('AD_SYSTEMS')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_BS_COUNT_URL')),
                        has_entries(key=valid_schema_field_descriptor('DISABLE_ADB_ENABLED')),
                        has_entries(key=valid_schema_field_descriptor('DISABLED_AD_TYPES')),
                        has_entries(key=valid_schema_field_descriptor('DISABLE_TGA_WITH_CREATIVES')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_ADB_FUNCTIONS')),
                        has_entries(key=valid_schema_field_descriptor('CRYPTED_URL_MIN_LENGTH')),
                        has_entries(key=valid_schema_field_descriptor('CRYPTED_URL_MIXING_TEMPLATE')),
                        has_entries(key=valid_schema_field_descriptor('IMAGE_URLS_CRYPTING_PROBABILITY')),
                        has_entries(key=valid_schema_field_descriptor('HOSTNAME_MAPPING')),
                        has_entries(key=valid_schema_field_descriptor('CRYPTED_HOST')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_URL_PREFFIX')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_URL_RANDOM_PREFFIXES')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_URL_OLD_PREFFIXES')),
                        has_entries(key=valid_schema_field_descriptor('ENCRYPT_TO_THE_TWO_DOMAINS')),
                        has_entries(key=valid_schema_field_descriptor('PARTNER_COOKIELESS_DOMAIN')),
                        has_entries(key=valid_schema_field_descriptor('PARTNER_TO_COOKIELESS_HOST_URLS_RE')),
                        has_entries(key=valid_schema_field_descriptor('AVATARS_NOT_TO_COOKIELESS')))),
        has_entries(group_name="FEATURES", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('INVERTED_COOKIE_ENABLED')),
                        has_entries(key=valid_schema_field_descriptor('DIV_SHIELD_ENABLE')),
                        has_entries(key=valid_schema_field_descriptor('RTB_AUCTION_VIA_SCRIPT')),
                        has_entries(key=valid_schema_field_descriptor('HIDE_META_ARGS_ENABLED')),
                        has_entries(key=valid_schema_field_descriptor('HIDE_META_ARGS_HEADER_MAX_SIZE')),
                        has_entries(key=valid_schema_field_descriptor('REMOVE_SCRIPTS_AFTER_RUN')),
                        has_entries(key=valid_schema_field_descriptor('ADD_NONCE')),
                        has_entries(key=valid_schema_field_descriptor('REMOVE_ATTRIBUTE_ID')),
                        has_entries(key=valid_schema_field_descriptor('UPDATE_RESPONSE_HEADERS_VALUES')),
                        has_entries(key=valid_schema_field_descriptor('UPDATE_REQUEST_HEADER_VALUES')),
                        has_entries(key=valid_schema_field_descriptor('DISABLE_SHADOW_DOM')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_ENABLE_TRAILING_SLASH')),
                        has_entries(key=valid_schema_field_descriptor('CRYPT_LINK_HEADER')),
                        has_entries(key=valid_schema_field_descriptor('HIDE_LINKS')),
                        has_entries(key=valid_schema_field_descriptor('VISIBILITY_PROTECTION_CLASS_RE')),
                        has_entries(key=valid_schema_field_descriptor('XHR_PROTECT')),
                        has_entries(key=valid_schema_field_descriptor('LOADED_JS_PROTECT')),
                        has_entries(key=valid_schema_field_descriptor('BREAK_OBJECT_CURRENT_SCRIPT')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_SCRIPT_WITH_XHR_RE')),
                        has_entries(key=valid_schema_field_descriptor('REPLACE_RESOURCE_WITH_XHR_SYNC_RE')),
                        has_entries(key=valid_schema_field_descriptor('IMAGE_TO_IFRAME_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('IMAGE_TO_IFRAME_URL_LENGTH')),
                        has_entries(key=valid_schema_field_descriptor('IMAGE_TO_IFRAME_URL_IS_RELATIVE')),
                        has_entries(key=valid_schema_field_descriptor('IMAGE_TO_IFRAME_CHANGING_PROBABILITY')),
                        has_entries(key=valid_schema_field_descriptor('USE_CACHE')),
                        has_entries(key=valid_schema_field_descriptor('NO_CACHE_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('BYPASS_BY_UIDS')),
                        has_entries(key=valid_schema_field_descriptor('COUNT_TO_XHR')),
                        has_entries(key=valid_schema_field_descriptor('INJECT_INLINE_JS')),
                        has_entries(key=valid_schema_field_descriptor('INJECT_INLINE_JS_POSITION')),
                        has_entries(key=valid_schema_field_descriptor('BLOCK_TO_IFRAME_SELECTORS')),
                        has_entries(key=valid_schema_field_descriptor('CSP_PATCH')),
                        has_entries(key=valid_schema_field_descriptor('ADDITIONAL_PCODE_PARAMS')),
                        has_entries(key=valid_schema_field_descriptor('NETWORK_FAILS_RETRY_THRESHOLD')))),
        has_entries(group_name="DETECT_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('DETECT_HTML')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_LINKS')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_TRUSTED_LINKS')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_IFRAME')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_COOKIE_TYPE')),
                        has_entries(key=valid_schema_field_descriptor('DETECT_COOKIE_DOMAINS')),
                        has_entries(key=valid_schema_field_descriptor('DISABLE_DETECT')),
                        has_entries(key=valid_schema_field_descriptor('NEW_DETECT_SCRIPT_URL')),
                        has_entries(key=valid_schema_field_descriptor('NEW_DETECT_SCRIPT_DC')),
                        has_entries(key=valid_schema_field_descriptor('AUTO_SELECT_DETECT')))),
        has_entries(group_name="COOKIEMATCHING_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('CM_TYPE')),
                        has_entries(key=valid_schema_field_descriptor('EXTUID_COOKIE_NAMES')),
                        has_entries(key=valid_schema_field_descriptor('EXTUID_TAG')),
                        has_entries(key=valid_schema_field_descriptor('CM_IMAGE_URL')),
                        has_entries(key=valid_schema_field_descriptor('CM_REDIRECT_URL')))),
        has_entries(group_name="INTERNAL_PARTNER_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=contains(
                        has_entries(key=valid_schema_field_descriptor('INTERNAL')),
                        has_entries(key=valid_schema_field_descriptor('SERVICE_SLB_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('PARTNER_BACKEND_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('BALANCERS_PROD')),
                        has_entries(key=valid_schema_field_descriptor('BALANCERS_TEST')))),
        has_entries(group_name="REDIRECT_SETTINGS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('FOLLOW_REDIRECT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('CLIENT_REDIRECT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('ACCEL_REDIRECT_URL_RE')),
                        has_entries(key=valid_schema_field_descriptor('PARTNER_ACCEL_REDIRECT_ENABLED')),
                        has_entries(key=valid_schema_field_descriptor('PROXY_ACCEL_REDIRECT_URL_RE')))),
        has_entries(group_name="DEBUG", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('ADFOX_DEBUG')),
                        has_entries(key=valid_schema_field_descriptor('AIM_BANNER_ID_DEBUG_VALUE')),
                        has_entries(key=valid_schema_field_descriptor('DEBUG_LOGGING_ENABLE')),
                        has_entries(key=valid_schema_field_descriptor('TEST_DATA')))),
        has_entries(group_name="EXPERIMENTS", title=is_(basestring), hint=is_(basestring),
                    items=only_contains(
                        has_entries(key=valid_schema_field_descriptor('EXPERIMENTS')))),
    ))


@pytest.mark.usefixtures("transactional")
def test_user_has_service_permissions_search(api, service, grant_permissions):
    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")
    r = session.get(api["search"], params=dict(pattern=r"PROXY_URL_RE"))
    assert r.status_code == 200
    assert r.json()["total"] == 1


@pytest.mark.usefixtures("transactional")
def test_user_has_no_service_permissions_search(api, service):
    session = Session(USER2_SESSION_ID)
    r = session.get(api["search"], params=dict(pattern=r"PROXY_URL_RE"))
    assert r.status_code == 200
    assert r.json()["total"] == 0


@pytest.mark.usefixtures("transactional")
def test_user_cant_generate_token(api, service):
    session = Session(USER2_SESSION_ID)
    response = session.get(api["service"][service["id"]]["gen_token"])
    assert response.status_code == 403


@pytest.mark.usefixtures("transactional")
def test_user_cant_service_check_in_progress(api, service):
    session = Session(USER2_SESSION_ID)
    r = session.patch(api['service']['auto.ru']['check']['4xx_errors']['in_progress'], json=dict())
    assert r.status_code == 403
