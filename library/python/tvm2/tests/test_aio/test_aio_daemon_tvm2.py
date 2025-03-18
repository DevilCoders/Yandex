import pytest


@pytest.mark.asyncio
async def test_get_service_tickets_aio_qloud_success(tvm_aio_qloud, test_vcr):
    with test_vcr.use_cassette('test_get_service_tickets_aio_qloud_success.yaml'):
        ticket = await tvm_aio_qloud.get_service_ticket('2000255')
    assert ticket == '3:serv:CLYQEKzezdAFIggIoIt6EP-Keg:XoHAcHbYqUkeATFQQ6Ll_'


@pytest.mark.asyncio
async def test_parse_service_ticket_aio_qloud_success(tvm_aio_qloud, test_vcr, valid_service_ticket):
    tvm_aio_qloud.allowed_clients = (2000324, )
    with test_vcr.use_cassette('test_parse_service_ticket_aio_qloud_success.yaml'):
        parsed_ticket = await tvm_aio_qloud.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 2000324
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )


@pytest.mark.asyncio
async def test_parse_service_ticket_aio_deploy_success(tvm_aio_deploy, test_vcr, valid_service_ticket):
    tvm_aio_deploy.allowed_clients = (2000324, )
    with test_vcr.use_cassette('test_parse_service_ticket_aio_deploy_success.yaml'):
        parsed_ticket = await tvm_aio_deploy.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 2000324
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )


@pytest.mark.asyncio
async def test_parse_user_ticket_invalid_ticket_aio_qloud_fail(tvm_aio_qloud, test_vcr,
                                                               invalid_user_ticket, caplog):
    with test_vcr.use_cassette('test_parse_user_ticket_invalid_ticket_aio_qloud_fail.yaml'):
        parsed_ticket = await tvm_aio_qloud.parse_user_ticket(invalid_user_ticket)
    assert parsed_ticket is None


@pytest.mark.asyncio
async def test_parse_user_ticket_aio_qloud_success(tvm_aio_qloud, test_vcr, valid_user_ticket):
    with test_vcr.use_cassette('test_parse_user_ticket_aio_qloud_success.yaml'):
        parsed_ticket = await tvm_aio_qloud.parse_user_ticket(valid_user_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.uids == [456, 123]
    assert parsed_ticket.default_uid == 456
    assert parsed_ticket.debug_info == (
        'ticket_type=user;expiration_time=9223372036854775807;'
        'scope=bb:sess1;scope=bb:sess2;default_uid=456;uid=456;uid=123;'
    )


@pytest.mark.asyncio
async def test_get_service_tickets_error_response_aio_qloud_fail(tvm_aio_qloud, test_vcr, caplog):
    with test_vcr.use_cassette('test_get_service_tickets_error_response_aio_qloud_fail.yaml'):
        tickets = await tvm_aio_qloud.get_service_tickets('2000233')
    assert tickets == {'2000233': None}


@pytest.mark.asyncio
async def test_get_service_tickets_aio_qloud_fail(tvm_aio_qloud, test_vcr, caplog):
    with test_vcr.use_cassette('test_get_service_tickets_aio_qloud_fail.yaml'):
        tickets = await tvm_aio_qloud.get_service_tickets('2000233')
    assert tickets == {'2000233': None}


@pytest.mark.asyncio
async def test_get_service_ticket_bulk_not_response_for_one_qloud_success(tvm_aio_qloud, test_vcr, caplog):
    cassette = test_vcr.use_cassette(
        'test_get_service_ticket_bulk_not_response_for_one_aio_qloud_success.yaml'
    )
    with cassette:
        tickets = await tvm_aio_qloud.get_service_tickets('2000255', '2000266')
    assert tickets == {
        '2000255': '3:serv:CLYQEKzezdAFIggIoIt6EP-Keg:XoHAcHbYqUkeATFQQ6Ll_',
        '2000266': None,
    }
