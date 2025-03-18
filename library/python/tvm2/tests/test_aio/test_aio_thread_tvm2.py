import pytest


@pytest.mark.asyncio
async def test_get_service_tickets_aio_success(tvm_aio_threaded, test_vcr):
    tvm_aio_threaded.destinations = {'2000255': 2000255}
    tvm_aio_threaded.destinations_values = {2000255}
    tickets = await tvm_aio_threaded.aio_get_service_tickets('2000255')
    assert tickets == {'2000255': 'Some service ticket'}


@pytest.mark.asyncio
async def test_get_service_ticket_aio_success(tvm_aio_threaded, test_vcr):
    tvm_aio_threaded.destinations = {'2000255': 2000255}
    tvm_aio_threaded.destinations_values = {2000255}
    ticket = await tvm_aio_threaded.get_service_ticket('2000255')
    assert ticket == 'Some service ticket'


@pytest.mark.asyncio
async def test_parse_service_ticket_wrong_client_aio_fail(tvm_aio_threaded, valid_service_ticket, caplog):
    result = await tvm_aio_threaded.parse_service_ticket(valid_service_ticket)
    assert result is None
    log_text = caplog.text
    if callable(log_text):
        log_text = log_text()
    assert 'Request from not allowed client was made: "229"' in log_text


@pytest.mark.asyncio
async def test_parse_service_ticket_aio_success(tvm_aio_threaded, valid_service_ticket):
    tvm_aio_threaded.allowed_clients = {229, }
    parsed_ticket = await tvm_aio_threaded.parse_service_ticket(valid_service_ticket)
    assert parsed_ticket is not None
    assert parsed_ticket.src == 229
    assert parsed_ticket.debug_info == (
        'ticket_type=serv;expiration_time=9223372036854775807;'
        'src=229;dst=28;scope=bb:sess1;scope=bb:sess2;'
    )
