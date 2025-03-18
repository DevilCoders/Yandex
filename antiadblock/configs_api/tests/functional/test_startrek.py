# coding=utf-8
import pytest

from hamcrest import assert_that, has_entries


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('service_name, expected_code, expected_message',
                         [('wrong_component', 400, u'Wrong component'),
                          ('yandex_news', 404, u'Ticket not found')])
def test_get_issue_error(api, session, service_name, expected_code, expected_message):
    response = session.get(api['service'][service_name]['trend_ticket'])

    assert response.json()['message'] == expected_message
    assert response.status_code == expected_code


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('expected_code, expected_message',
                         [(200, {'ticket_id': '123'})])
def test_get_issue_success(api, session, service, expected_code, expected_message):
    response = session.get(api['service'][service['id']]['trend_ticket'])

    assert response.json() == expected_message
    assert response.status_code == expected_code


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('service_id, params, expected_message',
                         [('wrong_component', {}, u'Ошибка валидации'),
                          ('wrong_component', {'type': 'some_other_trend'}, u'Ошибка валидации'),
                          ('wrong_component', {'type': 'negative_trend', 'datetime': 'string'}, u'Ошибка валидации'),
                          ('wrong_component', {'datetime': 12345678}, u'Ошибка валидации'),
                          ('wrong_component', {'type': 'negative_trend', 'datetime': 12345678}, u'Wrong component')])
def test_create_issue_error(api, session, service_id, params, expected_message):
    response = session.post(api['service'][service_id]['create_ticket'], json=params)

    assert response.json()['message'] == expected_message
    assert response.status_code == 400


@pytest.mark.usefixtures("transactional")
def test_create_issue_success(api, session):
    service = dict(service_id="yandex.startrek",
                   name="yandex_startrek",
                   domain="startrek.yandex.ru",
                   support_priority="major")
    r = session.post(api['service'], json=service)

    assert r.status_code == 201

    response = session.post(api['service'][service['service_id']]['create_ticket'], json={'type': 'negative_trend', 'datetime': 12345678})

    assert_that(response.json(), has_entries(ticket_id='123'))
    assert response.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_create_issue_already_exists(api, session):
    service = dict(service_id="yandex.startrek.trend",
                   name="yandex.startrek.trend",
                   domain="yandex.startrek.trend.ru",
                   support_priority="critical")
    r = session.post(api['service'], json=service)

    assert r.status_code == 201

    response = session.post(api['service'][service['service_id']]['create_ticket'], json={'type': 'negative_trend'})

    assert_that(response.json(), has_entries(ticket_id='456'))
    assert response.status_code == 208


@pytest.mark.usefixtures("transactional")
def test_create_issue_change_type(api, session):
    service = dict(service_id="yandex.startrek.trend.2",
                   name="yandex.startrek.trend.2",
                   domain="yandex.startrek.trend.2.ru",
                   support_priority="critical")
    r = session.post(api['service'], json=service)

    assert r.status_code == 201

    response = session.post(api['service'][service['service_id']]['create_ticket'], json={'type': 'money_drop'})

    assert_that(response.json(), has_entries(ticket_id='789'))
    assert response.status_code == 205
