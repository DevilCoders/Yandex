# coding: utf-8

"""
Тесты взаимодействия с tvm-демоном. Общий для qloud и deploy функционал.
"""

from __future__ import unicode_literals


def test_get_service_tickets_qloud_success(tvmqloud, test_vcr):
    with test_vcr.use_cassette('test_get_service_tickets_qloud_success.yaml'):
        ticket = tvmqloud.get_service_tickets('2000255')
    assert ticket == {'2000255': '3:serv:CLYQEKzezdAFIggIoIt6EP-Keg:XoHAcHbYqUkeATFQQ6Ll_'}


def test_parse_service_ticket_qloud_invalid_ticket_fail(tvmqloud, test_vcr,
                                                        invalid_service_ticket, caplog):
    with test_vcr.use_cassette('test_parse_service_ticket_qloud_invalid_ticket_fail.yaml'):
        result = tvmqloud.parse_service_ticket(invalid_service_ticket)
    assert result is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert '403 Client Error: Ok for url: http://localhost:1/tvm/checksrv?dst=28' in log_text


def test_parse_service_ticket_qloud_wrong_client_fail(tvmqloud, test_vcr,
                                                      valid_service_ticket, caplog):
    with test_vcr.use_cassette('test_parse_service_ticket_qloud_wrong_client_fail.yaml'):
        result = tvmqloud.parse_service_ticket(valid_service_ticket)
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Request from not allowed client was made: "2000324"' in log_text
    assert result is None


def test_parse_service_ticket_qloud_wrong_client_without_allowed_success(tvmqloud,
                                                                         test_vcr,
                                                                         valid_service_ticket,
                                                                         caplog):
    tvmqloud.allowed_clients = {'*'}
    with test_vcr.use_cassette('test_parse_service_ticket_qloud_success.yaml'):
        parsed_ticket = tvmqloud.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 2000324
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )
    tvmqloud.allowed_clients = set()


def test_parse_service_ticket_qloud_success(tvmqloud, test_vcr, valid_service_ticket):
    tvmqloud.allowed_clients = (2000324, )
    with test_vcr.use_cassette('test_parse_service_ticket_qloud_success.yaml'):
        parsed_ticket = tvmqloud.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 2000324
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )


def test_parse_user_ticket_invalid_ticket_qloud_fail(tvmqloud, test_vcr,
                                                     invalid_user_ticket, caplog):
    with test_vcr.use_cassette('test_parse_user_ticket_invalid_ticket_qloud_fail.yaml'):
        parsed_ticket = tvmqloud.parse_user_ticket(invalid_user_ticket)
    assert parsed_ticket is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'HTTPError: 403 Client Error' in log_text


def test_parse_user_ticket_qloud_success(tvmqloud, test_vcr, valid_user_ticket):
    with test_vcr.use_cassette('test_parse_user_ticket_qloud_success.yaml'):
        parsed_ticket = tvmqloud.parse_user_ticket(valid_user_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.uids == [456, 123]
    assert parsed_ticket.default_uid == 456
    assert parsed_ticket.debug_info == (
        'ticket_type=user;expiration_time=9223372036854775807;'
        'scope=bb:sess1;scope=bb:sess2;default_uid=456;uid=456;uid=123;'
    )


def test_get_service_tickets_error_response_qloud_fail(tvmqloud, test_vcr, caplog):
    with test_vcr.use_cassette('test_get_service_tickets_error_response_qloud_fail.yaml'):
        tickets = tvmqloud.get_service_tickets('2000233')
    assert tickets == {'2000233': None}
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert "HTTPError: 400 Client Error" in log_text


def test_get_service_tickets_qloud_fail(tvmqloud, test_vcr, caplog):
    with test_vcr.use_cassette('test_get_service_tickets_qloud_fail.yaml'):
        tickets = tvmqloud.get_service_tickets('2000233')
    assert tickets == {'2000233': None}
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert "Don't get tickets for all destinations" in log_text


def test_get_service_ticket_bulk_not_response_for_one_qloud_success(tvmqloud, test_vcr, caplog):
    cassette = test_vcr.use_cassette(
        'test_get_service_ticket_bulk_not_response_for_one_qloud_success.yaml'
    )
    with cassette:
        tickets = tvmqloud.get_service_tickets('2000255', '2000266')
    assert tickets == {
        '2000255': '3:serv:CLYQEKzezdAFIggIoIt6EP-Keg:XoHAcHbYqUkeATFQQ6Ll_',
        '2000266': None,
    }
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert "Don't get tickets for all destinations" in log_text


def test_get_service_ticket_bulk_qloud_success(tvmqloud, test_vcr):
    with test_vcr.use_cassette('test_get_service_ticket_bulk_qloud_success.yaml'):
        tickets = tvmqloud.get_service_tickets('2000255', '2000266')
    assert tickets == {
        '2000255': '3:serv:CLYQEKzezdAFIggIoIt6EP-Keg:XoHAcHbYqUkeATFQQ6Ll_',
        '2000266': '3:serv:VVVQEKzezdAFIggIoIt6EP-Keg:XoHAcHbYqUkeATFQQ6Ll_',
    }
