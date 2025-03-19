""" Examples could be got from https://juggler.yandex-team.ru/doc/#/downtimes//v2/downtimes/set_downtimes """
# import json
import pytest
import responses

# from yc_common.clients import juggler
from yc_common.clients.juggler import JugglerClient, GetDowntimesRequest, SetDowntimeRequest, DelDowntimesRequest, \
    DowntimeInfo, CommonFilter, ApiError, CheckInfo, GetCheckStatusRequest, CheckStatusFilter, StatusInfo
from yc_common.clients.api import _to_api_obj

TEST_JUGGLER_URL = "http://test-juggler"
TEST_JUGGLER_TOKEN = "AAAA-TEST-TOKEN"

JUGGLER_GET_DOWNTIMES_PATH = "/v2/downtimes/get_downtimes"
JUGGLER_SET_DOWNTIME_PATH = "/v2/downtimes/set_downtimes"
JUGGLER_DEL_DOWNTIMES_PATH = "/v2/downtimes/remove_downtimes"
JUGGLER_GET_CHECK_STATUS_PATH = "/v2/checks/get_checks_state"


def get_test_juggler_client():
    return JugglerClient(TEST_JUGGLER_URL, TEST_JUGGLER_TOKEN)


def mock_juggler_response(path, return_data_json, status=200):
    responses.add(responses.POST, TEST_JUGGLER_URL + path, json=return_data_json, status=status)


@responses.activate
def test_juggler_get_all_downtimes():
    test_1 = DowntimeInfo.new(downtime_id="dt1")
    test_2 = DowntimeInfo.new(downtime_id="dt2")
    mock_juggler_response(JUGGLER_GET_DOWNTIMES_PATH, {"items": _to_api_obj([test_1, test_2])})
    api = get_test_juggler_client()
    result = api.get_downtimes()
    assert result == [test_1, test_2]


@responses.activate
def test_juggler_get_check_status():
    test_check_info = CheckInfo(
        {'status': 'CRIT', 'description': '1/1 (0/0)', 'service': 'test service', 'aggregration_time': None,
         'change_time': 1570960842.368858, 'downtime_ids': [], 'host': 'test_host', 'meta': '{}'})
    test_status_info = StatusInfo({'count': 1, 'status': 'CRIT'})
    filter_check = CheckStatusFilter.new(service="test_service", host="test.ru")
    mock_juggler_response(JUGGLER_GET_CHECK_STATUS_PATH,
                          _to_api_obj({"items": [test_check_info], "statuses": [test_status_info]}))
    api = get_test_juggler_client()
    result = api.get_check_status(GetCheckStatusRequest.new(filters=[filter_check]))
    assert result == ([test_check_info], [test_status_info])


@responses.activate
def test_juggler_get_scecific_downtime():
    filter_dt = CommonFilter.new(host="test.ru")
    test_dt = DowntimeInfo.new(downtime_id="dt1", filters=[filter_dt])
    mock_juggler_response(JUGGLER_GET_DOWNTIMES_PATH, {"items": _to_api_obj([test_dt])})
    api = get_test_juggler_client()
    result = api.get_downtimes(GetDowntimesRequest.new(filters=[filter_dt]))
    assert len(result) == 1
    res_dt = result[0]
    assert res_dt == test_dt
    assert res_dt.filters == [filter_dt]


@responses.activate
def test_juggler_set_scecific_downtime():
    filter_dt = CommonFilter.new(host="test.ru")
    _ = DowntimeInfo.new(downtime_id="dt1", filters=[filter_dt])
    downtime_id = "test-id"
    mock_juggler_response(JUGGLER_SET_DOWNTIME_PATH, {"downtime_id": downtime_id})
    api = get_test_juggler_client()
    result = api.set_downtime(SetDowntimeRequest.new(filters=[filter_dt]))
    assert result == downtime_id


@responses.activate
def test_juggler_del_scecific_downtime():
    filter_dt = CommonFilter.new(host="test.ru")
    test_dt = DowntimeInfo.new(downtime_id="dt1", filters=[filter_dt])
    downtime_id = "test-id"
    mock_juggler_response(JUGGLER_DEL_DOWNTIMES_PATH, {"downtimes": _to_api_obj([test_dt])})
    api = get_test_juggler_client()
    result = api.del_downtimes(DelDowntimesRequest.new(downtime_ids=[downtime_id]))
    assert result == [test_dt]


@responses.activate
def test_juggler_to_big_page_size():
    MAX_PAGE_SIZE = 1000
    BIG_PAGE_SIZE = MAX_PAGE_SIZE + 1
    message = "page_size ({big}) cannot be larger than {max}".format(big=BIG_PAGE_SIZE, max=MAX_PAGE_SIZE)
    mock_juggler_response(JUGGLER_GET_DOWNTIMES_PATH,
                          {"message": message},
                          status=400)
    api = get_test_juggler_client()
    with pytest.raises(ApiError) as e:
        api.get_downtimes(GetDowntimesRequest.new(page_size=BIG_PAGE_SIZE))
        assert e.message == message
