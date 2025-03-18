# coding: utf-8

from __future__ import unicode_literals

from mock import patch, Mock

import blackbox

from tvmauth import BlackboxTvmId as BlackboxClientId
from tvmauth.mock import TvmClientPatcher, MockedTvmClient

from tvm2.sync.thread_tvm2 import TVM2
from tvm2.protocol import BLACKBOX_MAP


class MockBlackbox(object):
    def sessionid(self, *args, **kwargs):
        return {'user_ticket': '3:test_ticket', 'error': 'OK'}


def test_parse_service_ticket_wrong_client_fail(threaded_tvm, valid_service_ticket, caplog):
    result = threaded_tvm.parse_service_ticket(valid_service_ticket)
    assert result is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Request from not allowed client was made: "229"' in log_text


def test_parse_service_ticket_wrong_client_without_allowed_check_success(threaded_tvm,
                                                                         valid_service_ticket,
                                                                         caplog):
    threaded_tvm.allowed_clients = {'*', '123'}
    parsed_ticket = threaded_tvm.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 229
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )
    threaded_tvm.allowed_clients = set()


def test_parse_service_ticket_success(threaded_tvm, valid_service_ticket):
    threaded_tvm.allowed_clients = {229, }
    parsed_ticket = threaded_tvm.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 229
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )


def test_parse_service_ticket_invalid_ticket_fail(threaded_tvm, invalid_service_ticket, caplog):
    result = threaded_tvm.parse_service_ticket(invalid_service_ticket)
    assert result is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Got ticket parsing error while parsing ticket' in log_text


def test_parse_user_ticket_success(test_vcr, valid_user_ticket):
    TVM2._instance = None
    with TvmClientPatcher(MockedTvmClient(self_tvm_id=28)):
        tvm = TVM2(
            client_id='28',
            secret='GRMJrKnj4fOVnvOqe-WyD1',
            blackbox_client=BlackboxClientId.Test,
            retries=0,
        )
    parsed_ticket = tvm.parse_user_ticket(valid_user_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.uids == [456, 123]
    assert parsed_ticket.default_uid == 456
    assert parsed_ticket.debug_info == (
        'ticket_type=user;expiration_time=9223372036854775807;'
        'scope=bb:sess1;scope=bb:sess2;default_uid=456;uid=456;uid=123;env=Test;'
    )


def test_parse_user_ticket_invalid_ticket_fail(test_vcr, threaded_tvm,
                                               invalid_user_ticket, caplog):
    with test_vcr.use_cassette('tvm_keys.yaml'):
        parsed_ticket = threaded_tvm.parse_user_ticket(invalid_user_ticket)
    assert parsed_ticket is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Got ticket parsing error while parsing ticket' in log_text


def test_get_service_tickets_success(threaded_tvm):
    with patch.object(threaded_tvm, 'add_destinations') as patched_add:
        ticket = threaded_tvm.get_service_tickets('2000255')
    assert ticket == {'2000255': 'Some service ticket'}
    patched_add.assert_called_once_with(2000255)


def test_get_service_ticket_bulk_success(threaded_tvm, test_vcr):
    with patch.object(threaded_tvm, 'add_destinations') as patched_add:
        tickets = threaded_tvm.get_service_tickets('2000255', '28')
    assert tickets == {
        '2000255': 'Some service ticket',
        '28': 'Some service ticket',
    }
    patched_add.assert_called_once_with(2000255)


def test_get_service_ticket_bulk_success_with_int(threaded_tvm, test_vcr):
    with patch.object(threaded_tvm, 'add_destinations') as patched_add:
        tickets = threaded_tvm.get_service_tickets(2000255, '28')
    assert tickets == {
        2000255: 'Some service ticket',
        '28': 'Some service ticket',
    }
    patched_add.assert_called_once_with(2000255)


def test_get_user_ticket_success(threaded_tvm, test_vcr):
    with test_vcr.use_cassette('get_user_ticket.yaml'):
        with patch.object(TVM2, 'blackbox', new_callable=MockBlackbox):
            with patch.object(threaded_tvm, 'add_destinations') as patched_add:
                ticket = threaded_tvm.get_user_ticket(
                    session_id='session_value',
                    user_ip='95.108.172.236',
                    server_host='example.yandex-team.ru',
                )
    assert ticket == '3:test_ticket'


def test_get_user_ticket_blackbox_unavaliable_fail(threaded_tvm, test_vcr, caplog):
    with test_vcr.use_cassette('get_user_ticket.yaml'):
        with patch.object(TVM2, 'blackbox', new_callable=MockBlackbox) as mock_blackbox:
            with patch.object(threaded_tvm, 'add_destinations') as patched_add:
                mock_blackbox.sessionid = Mock(side_effect=blackbox.BlackboxError('error'))
                ticket = threaded_tvm.get_user_ticket(
                    session_id='session_value',
                    user_ip='95.108.172.236',
                    server_host='example.yandex-team.ru',
                )
    assert ticket is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Blackbox unavailable' in log_text


def test_get_user_ticket_no_ticket_fail(threaded_tvm, test_vcr, caplog):
    with test_vcr.use_cassette('get_user_ticket.yaml'):
        with patch.object(TVM2, 'blackbox', new_callable=MockBlackbox) as mock_blackbox:
            with patch.object(threaded_tvm, 'add_destinations') as patched_add:
                mock_blackbox.sessionid = Mock(return_value={'something': 'bad'})
                ticket = threaded_tvm.get_user_ticket(
                    session_id='session_value',
                    user_ip='95.108.172.236',
                    server_host='example.yandex-team.ru',
                )
    assert ticket is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Don\'t get user ticket in blackbox response' in log_text


def test_initialize_only_once(test_vcr):
    tvm_one = TVM2(
        client_id='28',
        secret='GRMJrKnj4fOVnvOqe-WyD1',
        allowed_clients=(229, ),
        blackbox_client=BlackboxClientId.Test,
    )
    tvm_two = TVM2(
        client_id='55',
        secret='test',
        allowed_clients=(123, ),
        blackbox_client=BlackboxClientId.Test,
    )
    tvm_three = TVM2()

    assert tvm_one is tvm_two is tvm_three
    assert tvm_one.client_id == tvm_two.client_id == tvm_three.client_id == 28
    assert (
        tvm_one.allowed_clients == tvm_two.allowed_clients
        == tvm_three.allowed_clients == {229, 123}
    )
    assert (
        tvm_one.blackbox_client == tvm_two.blackbox_client
        == tvm_three.blackbox_client == BlackboxClientId.Test
    )
    assert (
        tvm_one.blackbox_url == tvm_two.blackbox_url
        == tvm_three.blackbox_url == BLACKBOX_MAP[BlackboxClientId.Test]['url']
    )
    assert (
        tvm_one.blackbox_env == tvm_two.blackbox_env
        == tvm_three.blackbox_env == BLACKBOX_MAP[BlackboxClientId.Test]['env']
    )


def test_service_tickets_add_success(threaded_tvm, test_vcr):
    threaded_tvm.destinations = {'2000266': 2000266}
    threaded_tvm.destinations_values = {2000266}
    with TvmClientPatcher():
        threaded_tvm.add_destinations('2000255')
    assert threaded_tvm.destinations == {'2000255': 2000255, '2000266': 2000266}
    assert threaded_tvm.destinations_values == {2000255, 2000266}


def test_service_ticket_add_double_success(threaded_tvm, test_vcr):
    threaded_tvm.destinations = {'2000255': 2000255}
    threaded_tvm.destinations_values = {2000255}
    with TvmClientPatcher():
        threaded_tvm.add_destinations('2000255')
    assert threaded_tvm.destinations == {'2000255': 2000255}
    assert threaded_tvm.destinations_values == {2000255}


def test_get_service_ticket_add_destination_success(threaded_tvm, test_vcr):
    threaded_tvm.destinations = dict()
    threaded_tvm.destinations_values = set()
    with TvmClientPatcher():
        ticket = threaded_tvm.get_service_tickets('2000255')
    assert ticket == {'2000255': 'Some service ticket'}
    assert threaded_tvm.destinations == {'2000255': 2000255}
    assert threaded_tvm.destinations_values == {2000255}


def test_get_service_ticket_add_destination_to_existing_success(threaded_tvm, test_vcr):
    threaded_tvm.destinations = {'11111': 11111}
    threaded_tvm.destinations_values = {11111}
    with TvmClientPatcher():
        ticket = threaded_tvm.get_service_tickets('2000255')
    assert ticket == {'2000255': 'Some service ticket'}
    assert threaded_tvm.destinations == {'2000255': 2000255, '11111': 11111}
    assert threaded_tvm.destinations_values == {2000255, 11111}


def test_set_destinations_success(threaded_tvm, test_vcr):
    threaded_tvm.destinations = {'111': 111, '222': 222}
    threaded_tvm.destinations_values = {111, 222}
    with TvmClientPatcher():
        threaded_tvm.set_destinations('2000255', '2000266')

    assert threaded_tvm.destinations == {'2000255': 2000255, '2000266': 2000266}
    assert threaded_tvm.destinations_values == {2000255, 2000266}
