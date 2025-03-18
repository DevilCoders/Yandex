from copy import deepcopy

import pytest
from hamcrest import has_entries, contains, assert_that, is_

from conftest import ADMIN_USER_ID, ADMIN_LOGIN, Session, USER2_ID, USER2_SESSION_ID, USER2_LOGIN, \
    USER3_LOGIN, USER3_SESSION_ID, ADMIN_SESSION_ID

CREATION_LOG_ENTRY = has_entries(id=is_(int),
                                 user_id=ADMIN_USER_ID,
                                 user_name=ADMIN_LOGIN,
                                 action="service_create",
                                 date=is_(basestring),
                                 service_id=is_(basestring),
                                 params=has_entries(service_domain=is_(basestring),
                                                    service_name=is_(basestring)))


@pytest.mark.usefixtures("transactional")
def test_log_activate_config(api, session, service):
    label_id = service["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][label_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][label_id])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="config_mark_active",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    old_config_id=old_config["id"])),
                                                     CREATION_LOG_ENTRY),
                                      total=2))


@pytest.mark.usefixtures("transactional")
def test_log_test_config(api, session, service):
    label_id = service["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.put(api["label"][label_id]["config"][config["id"]]["test"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="config_mark_test",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    old_config_id=old_config["id"])),
                                                     CREATION_LOG_ENTRY),
                                      total=2))


@pytest.mark.usefixtures("transactional")
def test_log_active_different_services(api, session, service):
    label_id = service["id"]

    # create second service
    r = session.post(api['service'], json=dict(service_id="service.ru",
                                               name="another-well-known-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service = r.json()

    # create and activate config on first service
    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=old_config["data"], data_settings={}, comment="Config 2",
                               parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()

    r = session.put(api["label"][label_id]["config"][config["id"]]["active"], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200

    # create and activate config on second service
    r = session.get(api["label"][another_service["id"]]["configs"])
    assert r.status_code == 200
    old_another_config = r.json()["items"][0]

    r = session.post(api["label"][another_service["id"]]["config"],
                     json=dict(data=old_another_config["data"], data_settings={}, comment="Config 2",
                               parent_id=old_another_config["id"]))
    assert r.status_code == 201
    another_config = r.json()

    r = session.put(api["label"][another_service["id"]]["config"][another_config["id"]]["active"],
                    json=dict(old_id=old_another_config["id"]))
    assert r.status_code == 200

    # check audit log has only one record per service
    r = session.get(api["audit"]["service"][label_id])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="config_mark_active",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    old_config_id=old_config["id"])),
                                                     CREATION_LOG_ENTRY),
                                      total=2))


@pytest.mark.usefixtures("transactional")
def test_log_create_service(api, session, service):
    service_id = service["id"]

    r = session.get(api["audit"]["service"][service_id])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(CREATION_LOG_ENTRY),
                                      total=1))


@pytest.mark.usefixtures("transactional")
def test_log_archive_config(api, session, service):
    label_id = service["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="config_archive",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    archived=True)),
                                                     CREATION_LOG_ENTRY),
                                      total=2))

    r = session.patch(api["config"][config["id"]], json=dict(archived=False))
    assert r.status_code == 200
    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="config_archive",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    archived=False)),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="config_archive",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    archived=True)),
                                                     CREATION_LOG_ENTRY),
                                      total=3))


@pytest.mark.usefixtures("transactional")
def test_log_set_service_property(api, session, service):
    service_id = service["id"]
    r = session.post(api["service"][service_id]["set"], json=dict(monitorings_enabled=False))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_monitorings_switch_status",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 label_id=service_id,
                                                                 params=has_entries(monitorings_enabled=False)),
                                                     CREATION_LOG_ENTRY),
                                      total=2))

    r = session.post(api["service"][service_id]["set"], json=dict(monitorings_enabled=True))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_monitorings_switch_status",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 label_id=service_id,
                                                                 params=has_entries(monitorings_enabled=True)),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_monitorings_switch_status",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 label_id=service_id,
                                                                 params=has_entries(monitorings_enabled=False)),
                                                     CREATION_LOG_ENTRY),

                                      total=3))


@pytest.mark.usefixtures("transactional")
def test_log_service_status_switch(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["disable"])
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_status_switch",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 label_id=service_id,
                                                                 params=has_entries(status="inactive")),
                                                     CREATION_LOG_ENTRY),
                                      total=2))

    r = session.post(api["service"][service_id]["enable"])
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_status_switch",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 label_id=service_id,
                                                                 params=has_entries(status="ok")),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_status_switch",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 label_id=service_id,
                                                                 params=has_entries(status="inactive")),
                                                     CREATION_LOG_ENTRY),
                                      total=3))


@pytest.mark.usefixtures("transactional")
def test_log_hidden_admin_username(api, service, grant_permissions):
    label_id = service["id"]

    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name="ADMIN",
                                                                 action="service_create",
                                                                 date=is_(basestring),
                                                                 service_id=is_(basestring),
                                                                 params=has_entries(service_domain=is_(basestring),
                                                                                    service_name=is_(basestring)))),
                                      total=1))

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=USER2_ID,
                                                                 user_name=USER2_LOGIN,
                                                                 action="config_archive",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    archived=True)),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name="ADMIN",
                                                                 action="service_create",
                                                                 date=is_(basestring),
                                                                 service_id=is_(basestring),
                                                                 params=has_entries(service_domain=is_(basestring),
                                                                                    service_name=is_(basestring)))),
                                      total=2))


@pytest.mark.usefixtures("transactional")
def test_log_admin_see_another_admin(api, service, grant_permissions):
    service_id = service["id"]

    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "admin")

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="service_create",
                                                                 date=is_(basestring),
                                                                 service_id=is_(basestring),
                                                                 params=has_entries(service_domain=is_(basestring),
                                                                                    service_name=is_(basestring)))),
                                      total=1))


@pytest.mark.usefixtures("transactional")
def test_log_user_see_another_user(api, service, grant_permissions):
    label_id = service["id"]

    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service['id'], "external_user")

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][label_id]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    session = Session(USER3_SESSION_ID)
    grant_permissions(USER3_LOGIN, service['id'], "external_user")

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=USER2_ID,
                                                                 user_name=USER2_LOGIN,
                                                                 action="config_archive",
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 params=has_entries(config_id=config["id"],
                                                                                    archived=True)),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name="ADMIN",
                                                                 action="service_create",
                                                                 date=is_(basestring),
                                                                 service_id=is_(basestring),
                                                                 params=has_entries(service_domain=is_(basestring),
                                                                                    service_name=is_(basestring)))),
                                      total=2))


@pytest.mark.usefixtures("transactional")
def test_log_hidden_admin_service_by_webmaster(api, service_by_webmaster):
    session = Session(USER2_SESSION_ID)
    r = session.get(api["audit"]["service"][service_by_webmaster["id"]], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name="ADMIN",
                                                                 action="service_create",
                                                                 date=is_(basestring),
                                                                 service_id=is_(basestring),
                                                                 params=has_entries(service_domain=is_(basestring),
                                                                                    service_name=is_(basestring)))),
                                      total=1))

    r = session.get(api["label"][service_by_webmaster["id"]]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]

    config_data = deepcopy(old_config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    r = session.post(api["label"][service_by_webmaster["id"]]["config"],
                     json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    r = session.patch(api["config"][config["id"]], json=dict(archived=True))
    assert r.status_code == 200

    r = session.get(api["audit"]["service"][service_by_webmaster["id"]], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=USER2_ID,
                                                                 user_name=USER2_LOGIN,
                                                                 action="config_archive",
                                                                 date=is_(basestring),
                                                                 service_id=service_by_webmaster["id"],
                                                                 params=has_entries(config_id=config["id"],
                                                                                    archived=True)),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name="ADMIN",
                                                                 action="service_create",
                                                                 date=is_(basestring),
                                                                 service_id=is_(basestring),
                                                                 params=has_entries(service_domain=is_(basestring),
                                                                                    service_name=is_(basestring)))),
                                      total=2))


@pytest.mark.usefixtures("transactional")
def test_audit_limit_offset(api, session, service):
    label_id = service["id"]

    r = session.get(api["label"][label_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    parent_id = config["id"]
    for i in xrange(10):
        r = session.post(api["label"][label_id]["config"],
                         json=dict(data=config_data, data_settings={}, comment="Config {}".format(i),
                                   parent_id=parent_id))
        assert r.status_code == 201

        config_new = r.json()
        r_new = session.put(api["label"][label_id]["config"][config_new["id"]]["test"], json=dict(old_id=parent_id))
        assert r_new.status_code == 200

        parent_id = r.json()["id"]

    r = session.get(api["audit"]["service"][label_id], params=dict(offset=5, limit=6))
    assert r.status_code == 200
    assert r.json()["total"] == 11
    assert len(r.json()["items"]) == 6

    assert_that(r.json()["items"][5], CREATION_LOG_ENTRY)

    for item in r.json()["items"][:-1]:
        assert_that(item, has_entries(id=is_(int),
                                      user_id=ADMIN_USER_ID,
                                      user_name=ADMIN_LOGIN,
                                      action="config_mark_test",
                                      date=is_(basestring),
                                      service_id=label_id))


@pytest.mark.usefixtures("transactional")
def test_log_argus_create_sbs_profile(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=[dict(url="https://yandex.ru")])))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["profile"])
    assert r.status_code == 200
    new_profile_id = r.json()["id"]

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="sbs_profile_update",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 params=has_entries(new_profile_id=new_profile_id,
                                                                                    old_profile_id=0)),
                                                     CREATION_LOG_ENTRY),
                                      total=2))

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=[dict(url="https://google.com")])))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["profile"])
    assert r.status_code == 200
    old_profile_id = new_profile_id
    new_profile_id = r.json()["id"]

    r = session.get(api["audit"]["service"][service_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="sbs_profile_update",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 params=has_entries(new_profile_id=new_profile_id,
                                                                                    old_profile_id=old_profile_id)),
                                                     has_entries(id=is_(int),
                                                                 user_id=ADMIN_USER_ID,
                                                                 user_name=ADMIN_LOGIN,
                                                                 action="sbs_profile_update",
                                                                 date=is_(basestring),
                                                                 service_id=service_id,
                                                                 params=has_entries(new_profile_id=old_profile_id,
                                                                                    old_profile_id=0)),
                                                     CREATION_LOG_ENTRY),
                                      total=3))


@pytest.mark.usefixtures("transactional")
def test_audit_with_parents(api, session):
    # Create hierarchy: ROOT -> verybestlabel -> mail.yandex.ru
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="verybestlabel"))
    assert r.status_code == 201

    r = session.post(api['service'], json=dict(service_id="mail.yandex.ru",
                                               parent_label_id="verybestlabel",
                                               name="mail.yandex.ru",
                                               domain="mail.yandex.ru"))
    assert r.status_code == 201

    r = session.get(api["label"]["ROOT"]["configs"])
    assert r.status_code == 200
    old_config = r.json()["items"][0]
    config_data = deepcopy(old_config["data"])
    config_data['USE_CACHE'] = True
    config_data['CRYPT_URL_PREFFIX'] = '/_crpd/'
    r = session.post(api["label"]["ROOT"]["config"],
                     json=dict(data=config_data, data_settings={}, comment="ROOT Config 2", parent_id=old_config["id"]))
    assert r.status_code == 201
    config = r.json()
    # activate config
    r = session.put(api["label"]["ROOT"]["config"][config["id"]]['active'], json=dict(old_id=old_config["id"]))
    assert r.status_code == 200
    ROOT_CHANGE_LOG_ENTRY = has_entries(id=is_(int),
                                        user_id=ADMIN_USER_ID, user_name=ADMIN_LOGIN,
                                        action="config_mark_active",
                                        date=is_(basestring),
                                        service_id=None, label_id="ROOT",
                                        params=has_entries(config_id=config['id'], old_config_id=old_config['id']))
    LABEL_CREATION_LOG_ENTRY = has_entries(id=is_(int),
                                           user_id=ADMIN_USER_ID, user_name=ADMIN_LOGIN,
                                           action="label_create",
                                           date=is_(basestring),
                                           service_id=None, label_id="verybestlabel",
                                           params=has_entries(parent_label_id="ROOT"))
    r = session.get(api["audit"]["service"]['mail.yandex.ru'], params=dict(offset=0, limit=20))
    assert_that(r.json(),
                has_entries(items=contains(ROOT_CHANGE_LOG_ENTRY, CREATION_LOG_ENTRY, LABEL_CREATION_LOG_ENTRY),
                            total=3))

    r = session.get(api["audit"]["service"]['mail.yandex.ru'], params=dict(offset=0, limit=20, label_id="ROOT"))
    assert_that(r.json(),
                has_entries(items=contains(ROOT_CHANGE_LOG_ENTRY),
                            total=1))

    r = session.get(api["audit"]["service"]['mail.yandex.ru'], params=dict(offset=0, limit=20, label_id="verybestlabel"))
    assert_that(r.json(),
                has_entries(items=contains(LABEL_CREATION_LOG_ENTRY),
                            total=1))

    r = session.get(api["audit"]["service"]['mail.yandex.ru'], params=dict(offset=0, limit=20, label_id="mail.yandex.ru"))
    assert_that(r.json(),
                has_entries(items=contains(CREATION_LOG_ENTRY),
                            total=1))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("status", ("test", "active"))
def test_log_configs_service_api(api, service, tickets, status):
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

    session = Session(ADMIN_SESSION_ID)
    r = session.get(api["audit"]["service"][label_id], params=dict(offset=0, limit=20))
    assert r.status_code == 200
    assert_that(r.json(), has_entries(items=contains(has_entries(id=is_(int),
                                                                 user_id=0,
                                                                 user_name="Mr. Robot",
                                                                 action="config_mark_{}".format(status),
                                                                 date=is_(basestring),
                                                                 service_id=label_id,
                                                                 label_id=label_id,
                                                                 params=has_entries(config_id=config_id,
                                                                                    old_config_id=old_config_id)),
                                                     CREATION_LOG_ENTRY),
                                      total=2))
