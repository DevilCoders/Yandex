# coding=utf-8
from datetime import datetime
import pytest
from hamcrest import has_entries, assert_that, contains_inanyorder, has_key, only_contains, equal_to, has_items

from antiadblock.configs_api.lib.dashboard.config import DASHBOARD_API_CONFIG

from conftest import Session, ADMIN_LOGIN, USER2_SESSION_ID, USER2_LOGIN


valid_till = datetime.utcnow()
VALID_TILL = valid_till.replace(year=valid_till.year + 1).strftime("%Y-%m-%dT%TZ")

CHECK_TYPES_REQUIRED_FIELDS = {
    'solomon': ['alert_id'],
    'yql': ['query'],
    'stat': ['report']
}


def test_validate_checks_config():
    assert_that(DASHBOARD_API_CONFIG, has_key('groups'))
    for group in DASHBOARD_API_CONFIG['groups']:
        assert_that(group.keys(), contains_inanyorder(
            'checks',
            'group_id',
            'group_title',
            'group_ttl',
            'group_update_period'
        ))
        checks = group['checks']
        for check in checks:
            assert_that(check, has_items('check_type', 'check_id', 'check_args', 'description', 'check_title'))
            assert_that(check['check_args'].keys(), has_items(*CHECK_TYPES_REQUIRED_FIELDS.get(check['check_type'])))


def test_unique_checks_ids():
    checks_list = list()
    for group in DASHBOARD_API_CONFIG['groups']:
        for check in group['checks']:
            checks_list.append(check['check_id'])
    assert_that(len(checks_list), equal_to(len(set(checks_list))))


@pytest.mark.usefixtures("transactional")
def test_check_unique_of_groupid_in_get_config(api, session, tickets):
    r = session.get(api['get_checks_config'], headers={'X-Ya-Service-Ticket': tickets.MONRELY_TICKET})
    refacted_groups = r.json()['groups']

    for group in DASHBOARD_API_CONFIG['groups']:
        list_refacted_group = filter(lambda another_group: another_group['group_id'] == group['group_id'], refacted_groups)
        # Если добавили группу с таким же group_id или не нашли
        assert_that(len(list_refacted_group), equal_to(1))


@pytest.mark.usefixtures("transactional")
def test_check_unique_of_checkid_in_get_config(api, session, tickets):
    r = session.get(api['get_checks_config'], headers={'X-Ya-Service-Ticket': tickets.MONRELY_TICKET})
    refacted_groups = r.json()['groups']

    for group in DASHBOARD_API_CONFIG['groups']:
        list_refacted_group = filter(lambda another_group: another_group['group_id'] == group['group_id'], refacted_groups)
        group_new = list_refacted_group[0]

        for check in group.get('checks'):
            list_refacted_checks = filter(lambda another_check: another_check['check_id'] == check['check_id'], group_new.get('checks'))
            # если есть одинаковые check_id
            assert_that(len(list_refacted_checks), equal_to(1))


@pytest.mark.usefixtures("transactional")
def test_get_checks_config(api, session, tickets):
    r = session.get(api['get_checks_config'], headers={'X-Ya-Service-Ticket': tickets.MONRELY_TICKET})
    refacted_groups = r.json()['groups']
    for group in DASHBOARD_API_CONFIG['groups']:
        list_refacted_group = filter(lambda another_group: another_group['group_id'] == group['group_id'], refacted_groups)

        group_new = list_refacted_group[0]
        assert_that(group_new.get('group_title'), equal_to(group.get('group_title')))

        for check in group.get('checks'):
            list_refacted_checks = filter(lambda another_check: another_check['check_id'] == check['check_id'], group_new.get('checks'))
            check_in_group_new = list_refacted_checks[0]

            assert_that(check_in_group_new, equal_to(dict(
                check_id=check.get('check_id'),
                check_type=check.get('check_type'),
                check_title=check.get('check_title'),
                check_args=check.get('check_args'),
                description=check.get('description'),
                ttl=check.get('ttl', group.get('group_ttl')),
                update_period=check.get('update_period', group.get('group_update_period'))
            )))


@pytest.mark.usefixtures("transactional")
@pytest.mark.skip(reason='Create more testcases after adding some real checks into DASHBOARD_API_CONFIG')
def test_get_service_checks(api, session, tickets):
    # TODO: Create more testcases after adding some real checks into DASHBOARD_API_CONFIG
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}

    data = [{'service_id': 'auto.ru', 'group_id': 'errors', 'check_id': '4xx_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00', 'valid_till': '2019-03-05 14:20:00'},
            {'service_id': 'auto.ru', 'group_id': 'errors', 'check_id': '5xx_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:30:00', 'valid_till': '2019-03-05 13:20:00'},
            ]
    session.post(api['post_service_checks'], headers=headers, json=data)

    response = session.get(api['service']['auto.ru']['get_service_checks']).json()

    assert len(response.get('groups', list())) == 1
    assert (response.get('groups')[0]).get('group_id') == 'errors'
    assert len((response.get('groups')[0]).get('checks', list())) == 2

    checks = list(map(lambda c: c.get('check_id'), response.get('groups')[0].get('checks', list())))
    assert '4xx_errors' in checks
    assert '5xx_errors' in checks
    assert 'rps_errors' not in checks

    response = session.get(api['service']['yandex.ru']['get_service_checks']).json()

    assert len((response.get('groups')[0]).get('checks', list())) == 0


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("data, use_ticket, expected_code, msg",
                         (({'test': 'test'}, True, 400, 'Invalid data format'),
                          ({'test': 'test'}, False, 403, 'Bad service ticket')))
def test_service_checks_bad_request(api, session, tickets, data, use_ticket, expected_code, msg):
    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET} if use_ticket else {}
    r = session.post(api['post_service_checks'], headers=headers, json=data)

    assert r.status_code == expected_code
    assert r.json()['message'] == msg


@pytest.mark.usefixtures("transactional")
def test_one_service_checks_append(api, session, tickets):
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    yellow_check_last_update = datetime.strptime('2019-03-05T13:20:00', '%Y-%m-%dT%H:%M:%S')
    green_check_last_update = datetime.strptime('2019-03-05T13:30:00', '%Y-%m-%dT%H:%M:%S')
    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '4xx_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path',
             'last_update': yellow_check_last_update.strftime('%Y-%m-%dT%H:%M:%S'),
             'valid_till': '2019-03-05T14:20:00.482000Z'},
            {'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '5xx_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path',
             'last_update': green_check_last_update.strftime('%Y-%m-%dT%H:%M:%S'),
             'valid_till': '2019-03-05T13:20:00.482000Z'},
            {'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': 'rps_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path',
             'last_update': green_check_last_update.strftime('%Y-%m-%dT%H:%M:%S'),
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.get(api["get_all_checks"])
    assert r.status_code == 200

    data = r.json()['items']['auto.ru']

    assert_that(data, contains_inanyorder(
        has_entries(state='green', outdated=0, name='rps_errors',
                    transition_time=int(green_check_last_update.strftime('%s')), in_progress=False),
        has_entries(state='green', outdated=1, name='5xx_errors',
                    transition_time=int(green_check_last_update.strftime('%s')), in_progress=False),
        has_entries(state='yellow', outdated=1, name='4xx_errors',
                    transition_time=int(yellow_check_last_update.strftime('%s')), in_progress=False),
    ))

    assert r.json()['total'] == 1


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("monitorings_enabled, items_keys", ((True, ['auto.ru']), (False, [])))
def test_one_service_get_all_checks_with_monitorings_enabled(api, session, tickets, monitorings_enabled, items_keys):
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '4xx_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05T13:20:00.482000Z',
             'valid_till': '2019-03-05T14:20:00.482000Z'},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.post(api['service']['auto.ru']['set'], json=dict(monitorings_enabled=monitorings_enabled))
    assert r.status_code == 200

    r = session.get(api['get_all_checks'])
    assert r.status_code == 200

    assert_that(r.json()['items'].keys(), contains_inanyorder(*items_keys))


@pytest.mark.usefixtures("transactional")
def test_one_service_checks_append_bad_data(api, session, tickets):
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '4xx_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': VALID_TILL},
            {'service_id': 'auto.ru', 'check_id': '5xx_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:30:00',
             'valid_till': '2019-03-05 13:20:00'},
            {'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': 'rps_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': [100, 200],
             'valid_till': '2019-03-05 14:20:00'},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 500

    r = session.get(api["get_all_checks"])
    assert r.status_code == 200

    data = r.json()['items']['auto.ru']
    assert data == []
    assert r.json()['total'] == 1


@pytest.mark.usefixtures("transactional")
def test_two_service_checks_append(api, session, tickets):
    response = session.post(api['service'], json=dict(service_id="yandex.ru",
                                                      name="morda",
                                                      domain="yandex.ru"))
    assert response.status_code == 201
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '4xx_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': VALID_TILL},
            {'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '5xx_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:30:00',
             'valid_till': '2019-03-05 13:20:00'},
            {'service_id': 'yandex.ru', 'group_id': 'Misc', 'check_id': 'fraud_fraud_money_percent', 'state': 'red', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': '2019-03-05 14:20:00'},
            {'service_id': 'yandex.ru', 'group_id': 'RPS', 'check_id': 'rps_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:30:00',
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.get(api["get_all_checks"])
    assert r.status_code == 200

    data = r.json()['items']['auto.ru']
    ts = int(datetime.strptime('2019-03-05 13:20:00', '%Y-%m-%d %H:%M:%S').strftime('%s'))
    ts2 = int(datetime.strptime('2019-03-05 13:30:00', '%Y-%m-%d %H:%M:%S').strftime('%s'))

    assert_that(data, contains_inanyorder(
        has_entries(state='yellow', outdated=0, name='4xx_errors', transition_time=ts, in_progress=False),
        has_entries(state='green', outdated=1, name='5xx_errors', transition_time=ts2, in_progress=False),
    ))

    data = r.json()['items']['yandex.ru']
    assert_that(data, contains_inanyorder(
        has_entries(state='red', outdated=1, name='fraud_fraud_money_percent', transition_time=ts, in_progress=False),
        has_entries(state='green', outdated=0, name='rps_errors', transition_time=ts2, in_progress=False),
    ))

    assert r.json()['total'] == 2


@pytest.mark.usefixtures("transactional")
def test_service_checks_update(api, session, tickets):
    response = session.post(api['service'], json=dict(service_id="auto.ru",
                                                      name="autoru",
                                                      domain="auto.ru"))
    assert response.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '4xx_errors', 'state': 'yellow', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': '2019-03-05 14:20:00'},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.get(api["get_all_checks"])
    assert r.status_code == 200

    data = r.json()['items']['auto.ru']
    ts = int(datetime.strptime('2019-03-05 13:20:00', '%Y-%m-%d %H:%M:%S').strftime('%s'))
    assert_that(data, contains_inanyorder(
        has_entries(state='yellow', outdated=1, name='4xx_errors', transition_time=ts, in_progress=False),
    ))

    assert r.json()['total'] == 1

    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': '4xx_errors', 'state': 'green', 'value': '2',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:30:00',
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.get(api["get_all_checks"])
    assert r.status_code == 200

    data = r.json()['items']['auto.ru']
    ts = int(datetime.strptime('2019-03-05 13:30:00', '%Y-%m-%d %H:%M:%S').strftime('%s'))
    assert_that(data, contains_inanyorder(
        has_entries(state='green', outdated=0, name='4xx_errors', transition_time=ts, in_progress=False),
    ))

    assert r.json()['total'] == 1


@pytest.mark.usefixtures("transactional")
def test_service_checks_in_progress(api, session, tickets):
    r = session.post(api['service'], json=dict(service_id="auto.ru", name="autoru", domain="auto.ru"))
    assert r.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    check_id = '4xx_errors'
    data = [{'service_id': 'auto.ru', 'group_id': 'RPS', 'check_id': check_id, 'state': 'red', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.patch(api['service']['auto.ru']['check'][check_id]['in_progress'], json=dict())
    assert r.status_code == 201

    r = session.get(api["get_all_checks"])
    assert r.status_code == 200

    data = r.json()['items']['auto.ru']
    ts = int(datetime.strptime('2019-03-05 13:20:00', '%Y-%m-%d %H:%M:%S').strftime('%s'))
    assert_that(data, only_contains(
        has_entries(state='red', outdated=0, name=check_id, transition_time=ts, in_progress=True),
    ))

    assert r.json()['total'] == 1


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("json, check_id, expected_code",
                         ((dict(hours=0), '4xx_errors', 400),
                          (dict(hours=169), '4xx_errors', 400),
                          (dict(), '5xx', 404),
                          ))
def test_service_checks_in_progress_bad_request(api, session, tickets, json, check_id, expected_code):
    r = session.post(api['service'], json=dict(service_id="auto.ru", name="autoru", domain="auto.ru"))
    assert r.status_code == 201

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    data = [{'service_id': 'auto.ru', 'group_id': 'errors', 'check_id': '4xx_errors', 'state': 'red', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.patch(api['service']['auto.ru']['check'][check_id]['in_progress'], json=json)
    assert r.status_code == expected_code


@pytest.mark.usefixtures("transactional")
def test_service_checks_in_progress_hidden_login(api, session, tickets, grant_permissions, service):
    service_id = service["id"]

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    check_id = '4xx_errors'
    data = [{'service_id': service_id, 'group_id': 'errors', 'check_id': check_id, 'state': 'red', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    response = session.get(api['service'][service_id]['get_service_checks'])
    assert response.status_code == 200

    r = session.patch(api['service'][service_id]['check'][check_id]['in_progress'], json=dict())
    assert r.status_code == 201

    def get_login(groups):
        for group in groups:
            if group["group_id"] != "errors":
                continue
            return group["checks"][0]["progress_info"]["login"]

    groups = session.get(api['service'][service_id]['get_service_checks']).json().get("groups", [])

    login = get_login(groups)
    assert login == ADMIN_LOGIN

    session = Session(USER2_SESSION_ID)
    grant_permissions(USER2_LOGIN, service_id, "external_user")

    groups = session.get(api['service'][service_id]['get_service_checks']).json().get("groups", [])
    login = get_login(groups)
    assert login == "ADMIN"


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("monitorings_enabled, len_of_groups", ((True, 1), (False, 0)))
def test_service_checks_with_monitorings_enabled(api, session, tickets, monitorings_enabled, service, len_of_groups):
    service_id = service["id"]

    headers = {'X-Ya-Service-Ticket': tickets.MONRELY_TICKET}
    data = [{'service_id': service_id, 'group_id': 'errors', 'check_id': '4xx_errors', 'state': 'red', 'value': '100',
             'external_url': 'https://www.some_url.ru/path', 'last_update': '2019-03-05 13:20:00',
             'valid_till': VALID_TILL},
            ]
    r = session.post(api['post_service_checks'], headers=headers, json=data)
    assert r.status_code == 201

    r = session.post(api['service'][service_id]['set'], json=dict(monitorings_enabled=monitorings_enabled))
    assert r.status_code == 200

    r = session.get(api['service'][service_id]['get_service_checks'])
    assert r.status_code == 200

    assert_that(len(r.json()["groups"]), equal_to(len_of_groups))
