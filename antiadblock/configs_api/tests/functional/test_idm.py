# coding=utf-8
import json

import pytest
from hamcrest import assert_that, has_entries, contains, contains_inanyorder, only_contains

from conftest import USER2_LOGIN, USER3_LOGIN, ADMIN_LOGIN, USER3_ANOTHER_LOGIN


VALID_ROLES = has_entries(name=has_entries(en="Choose role", ru=u"Выберите роль"),
                          slug="role",
                          values=has_entries(admin=has_entries(set="admin",
                                                               name=has_entries(en="Administrator", ru=u"Администратор")),
                                             internal_user=has_entries(set="internal_user",
                                                                       name=has_entries(en="User", ru=u"Пользователь")),
                                             observer=has_entries(set="observer",
                                                                  name=has_entries(en="Observer", ru=u"Наблюдатель"))))

VALID_FIELDS = contains(has_entries(required=True,
                                    type="passportlogin",
                                    name=has_entries(en="Passport login", ru=u"Паспортный логин"),
                                    slug="passport-login"))


def get_valid_project_roles(project):
    return has_entries(name=project, roles=VALID_ROLES)


def get_valid_user_role(node, role, login, p_login):
    result = dict(login=login)
    if node == "all":
        roles = {"project": node, "role": role}
    else:
        roles = {"project": "other", "service": node, "role": role}
    result["roles"] = contains(contains_inanyorder(has_entries(roles),
                                                   has_entries({"passport-login": p_login})))
    return has_entries(result)


def get_valid_user_two_role(node1, role1, node2, role2, login, p_login):
    result = dict(login=login)
    if node1 == "all":
        roles1 = {"project": node1, "role": role1}
    else:
        roles1 = {"project": "other", "service": node1, "role": role1}
    if node2 == "all":
        roles2 = {"project": node2, "role": role2}
    else:
        roles2 = {"project": "other", "service": node2, "role": role2}
    result["roles"] = contains(contains_inanyorder(has_entries(roles1),
                                                   has_entries({"passport-login": p_login})),
                               contains_inanyorder(has_entries(roles2),
                                                   has_entries({"passport-login": p_login})),
                               )
    return has_entries(result)


@pytest.mark.usefixtures("transactional")
def test_get_info(api, session, service, tickets):
    r = session.get(api["idm"]["info"], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      roles=has_entries(name=has_entries(en="Choose type of role", ru=u"Выберите тип роли"),
                                                        slug="project",
                                                        values=has_entries({"all": has_entries(name=dict(en="General", ru=u"Общие"),
                                                                                               help=dict(en="Global roles", ru=u"Глобальные роли"),
                                                                                               roles=VALID_ROLES),
                                                                            "other": has_entries(name=dict(en="Service", ru=u"Сервис"),
                                                                                                 help=dict(en="Roles on service", ru=u"Роли по сервисам"),
                                                                                                 roles=has_entries(name=has_entries(en="Choose service", ru=u"Выберите сервис"),
                                                                                                                   slug="service",
                                                                                                                   values=has_entries({"auto.ru": get_valid_project_roles("autoru")})))})),
                                      fields=VALID_FIELDS))


@pytest.mark.usefixtures("transactional")
def test_get_info_no_ticket(api, session, service):
    r = session.get(api["idm"]["info"], headers={'X-Ya-Service-Ticket': ""})
    assert r.status_code == 403


@pytest.mark.usefixtures("transactional")
def test_get_all_role(api, session, tickets):
    r = session.get(api["idm"]["get-all-roles"], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0, users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_get_all_role_no_ticket(api, session):
    r = session.get(api["idm"]["get-all-roles"], headers={'X-Ya-Service-Ticket': ""})
    assert r.status_code == 403


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('node, role, login, p_login, expected_code, return_code',
                         [("all", "external_user", USER3_LOGIN, USER3_LOGIN, 0, 200),
                          ("all", "external_user", USER3_LOGIN, "bad passport login", 1, 200),
                          ("all", "invalid_role", USER3_LOGIN, USER3_LOGIN, 1, 200),
                          ("bad service", "external_user", USER3_LOGIN, USER3_LOGIN, 1, 200)])
def test_add_one_role(api, session, node, role, login, p_login, expected_code, tickets, return_code):
    if node in ["*", "all"]:
        roles = json.dumps({"project": node, "role": role})
        path = "/project/{0}/role/{1}/".format(node, role)
    else:
        roles = json.dumps({"project": "other", "service": node, "role": role})
        path = "/project/other/service/{0}/role/{1}/".format(node, role)

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=login,
                               role=roles,
                               path=path,
                               fields=json.dumps({"passport-login": p_login})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == return_code
    if return_code == 200:
        assert r.json()["code"] == expected_code

        r = session.get(api["idm"]["get-all-roles"], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
        assert r.status_code == 200
        if expected_code == 0:
            assert_that(r.json(), has_entries(code=0,
                                              users=contains_inanyorder(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                                        get_valid_user_role(node, role, login, p_login))))
        else:
            assert_that(r.json(), has_entries(code=0,
                                              users=contains_inanyorder(
                                                  get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_add_one_role_no_ticket(api, session):
    node = "all"
    role = "external_user"

    roles = json.dumps({"project": node, "role": role})
    path = "/project/{0}/role/{1}/".format(node, role)

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER3_LOGIN,
                               role=roles,
                               path=path,
                               fields=json.dumps({"passport-login": USER3_LOGIN})),
                     headers={'X-Ya-Service-Ticket': ""})
    assert r.status_code == 403


@pytest.mark.usefixtures("transactional")
def test_add_two_role(api, session, service, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "other", "service": service["id"], "role": "admin"}),
                               path="/project/other/service/{}/role/observer/".format(service["id"]),
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=contains_inanyorder(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                                get_valid_user_two_role("all", "observer", service["id"], "admin", USER2_LOGIN, USER2_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_add_same_role(api, session, service, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                          get_valid_user_role("all", "observer", USER2_LOGIN, USER2_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_add_role_another_login(api, session, service, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER3_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER3_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER3_LOGIN,
                               role=json.dumps({"project": "all", "role": "internal_user"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER3_ANOTHER_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 1

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                          get_valid_user_role("all", "observer", USER3_LOGIN, USER3_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_update_role(api, session, service, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "internal_user"}),
                               path="/project/all/role/internal_user/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                          get_valid_user_role("all", "internal_user", USER2_LOGIN, USER2_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_remove_another_role(api, session, tickets):
    r = session.post(api["idm"]["remove-role"][""],
                     data=dict(login=ADMIN_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": ADMIN_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0
    r = session.get(api["idm"]["get-all-roles"], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_remove_role_not_permitted_user(api, session, tickets):
    r = session.post(api["idm"]["remove-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0
    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_remove_role(api, session, service, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "other", "service": service["id"], "role": "admin"}),
                               path="/project/other/service/{}/role/admin/".format(service["id"]),
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=contains_inanyorder(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                                get_valid_user_two_role("all", "observer", service["id"], "admin", USER2_LOGIN, USER2_LOGIN))))
    r = session.post(api["idm"]["remove-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "other", "service": service["id"], "role": "admin"}),
                               path="/project/other/service/{}/role/admin/".format(service["id"]),
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0
    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=contains_inanyorder(
                                          get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                          get_valid_user_role("all", "observer", USER2_LOGIN, USER2_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_remove_roles_fired_user(api, session, service, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "other", "service": service["id"], "role": "admin"}),
                               path="/project/other/service/{}/role/admin/".format(service["id"]),
                               fields=json.dumps({"passport-login": USER2_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=contains_inanyorder(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                                get_valid_user_two_role("all", "observer", service["id"], "admin", USER2_LOGIN, USER2_LOGIN))))
    r = session.post(api["idm"]["remove-role"][""],
                     data=dict(login=USER2_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER2_LOGIN}),
                               fired=1),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0
    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(
                                          get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN))))


@pytest.mark.usefixtures("transactional")
def test_add_role_another_login_after_remove(api, session, tickets):
    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER3_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER3_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                          get_valid_user_role("all", "observer", USER3_LOGIN, USER3_LOGIN))))

    r = session.post(api["idm"]["remove-role"][""],
                     data=dict(login=USER3_LOGIN,
                               role=json.dumps({"project": "all", "role": "observer"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER3_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.post(api["idm"]["add-role"][""],
                     data=dict(login=USER3_LOGIN,
                               role=json.dumps({"project": "all", "role": "internal_user"}),
                               path="/project/all/role/observer/",
                               fields=json.dumps({"passport-login": USER3_ANOTHER_LOGIN})),
                     headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["code"] == 0

    r = session.get(api["idm"]["get-all-roles"][""], headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(code=0,
                                      users=only_contains(get_valid_user_role("all", "admin", ADMIN_LOGIN, ADMIN_LOGIN),
                                                          get_valid_user_role("all", "internal_user", USER3_LOGIN, USER3_ANOTHER_LOGIN))))
