import pytest
import mock
import requests

from library.python.ok_client.structure import CreateApprovementRequest, StageSimple
from library.python.ok_client.client import OkClient
from library.python.ok_client.exception import UnretriableError, ApprovementAlreadyExists


def test_construct_url():

    ok = OkClient(token="test_construct_url", base_url="https://something.org")

    assert ok.construct_url("abc") == "https://something.org/abc/"
    assert ok.construct_url("john", "paul", "george", "ringo") == "https://something.org/john/paul/george/ringo/"
    assert ok.construct_url("") == "https://something.org/"
    assert ok.construct_url("", "", "") == "https://something.org/"
    assert ok.construct_url() == "https://something.org/"


@pytest.mark.parametrize(
    "base_url",
    ["https://something.org", "http://ya.ru", "abracadabra"],
)
def test_base_url(base_url):
    assert OkClient(token="111", base_url=base_url).base_url == base_url


def _get_response(status_code):
    response = requests.Response()
    response.status_code = status_code
    return response


def test_ok_client_response_handling__create__approvement_already_exists(mocked_ok_client, existing_approvements):
    for approvement in existing_approvements:
        with pytest.raises(ApprovementAlreadyExists):
            mocked_ok_client.create_approvement(
                payload=CreateApprovementRequest(
                    type="tracker",
                    object_id=approvement["object_id"],
                    text="Test OK client approvement basic",
                    stages=[
                        StageSimple(approver="user_1"),
                        StageSimple(approver="user_2"),
                        StageSimple(approver="user_3"),
                    ],
                    author="approvement_author",
                    groups=["test"],
                    is_parallel=True,
                ),
            )


def test_ok_client_response_handling__create__bad_request(mocked_ok_client):
    with pytest.raises(UnretriableError):

        mocked_ok_client.create_approvement(payload={})
        mocked_ok_client.create_approvement(payload={"absolutely": "useless"})
        mocked_ok_client.create_approvement(
            payload=CreateApprovementRequest(
                type="tracker",
                object_id="",
                text="Test OK client approvement basic",
                stages=[
                    StageSimple(approver="user_1"),
                    StageSimple(approver="user_2"),
                    StageSimple(approver="user_3"),
                ],
                author="approvement_author",
                groups=["test"],
                is_parallel=True,
            ),
        )


@mock.patch("library.python.ok_client.client.requests.post")
def test_create_approvement_request(
    requests_post_mock,
    create_approvement_successful_response,
    mocked_ok_client,
):

    requests_post_mock.return_value = create_approvement_successful_response

    payload = CreateApprovementRequest(
        type="tracker",
        object_id="TEST-1",
        text="Test OK client approvement basic",
        stages=[
            StageSimple(approver="user_1"),
            StageSimple(approver="user_2"),
            StageSimple(approver="user_3"),
        ],
        author="approvement_author",
        groups=["test"],
        is_parallel=True,
    )

    mocked_ok_client.create_approvement(payload=payload)

    requests_post_mock.assert_called_once()

    post_args, post_kwargs = requests_post_mock.call_args_list[-1]

    url = post_kwargs.pop("url") or post_args[0]
    headers = post_kwargs.pop("headers")
    timeout = post_kwargs.pop("timeout")

    assert url == mocked_ok_client.construct_url(mocked_ok_client.URL_PATH_APPROVEMENTS)
    assert headers
    assert "Authorization" in headers
    assert timeout


def test_ok_client_response_handling__create__success(mocked_ok_client):

    payload = CreateApprovementRequest(
        type="tracker",
        object_id="TESTOKSUCCESS-1",
        text="Test OK client approvement basic",
        stages=[
            StageSimple(approver="user_1"),
            StageSimple(approver="user_2"),
            StageSimple(approver="user_3"),
        ],
        author="approvement_author",
        groups=["test"],
        is_parallel=True,
    )

    try:

        mocked_ok_client.create_approvement(
            payload=payload,
        )

    except (UnretriableError, requests.HTTPError) as err:
        assert False, f"Unexpected error raised for a valid post request\nPayload: {payload}\nError: {err}"


def test_ok_client_response_handling__get__success(mocked_ok_client, existing_approvements):
    for approvement in existing_approvements:
        try:
            mocked_ok_client.get_approvement(
                uuid=approvement["uuid"],
            )
        except (UnretriableError, requests.HTTPError) as err:

            assert False, f"Unexpected error raised for a valid get request\nUUID: {approvement['uuid']}\nError: {err}"


def test_check_status_code():

    with pytest.raises(requests.HTTPError):
        OkClient.check_response_status_code(_get_response(404))
        OkClient.check_response_status_code(_get_response(500))

    with pytest.raises(UnretriableError):
        for err_code in OkClient.UNRETRIABLE_STATUS_CODES:
            OkClient.check_response_status_code(_get_response(err_code))

    try:
        for status_code in range(200, 209):
            OkClient.check_response_status_code(_get_response(status_code))
    except (requests.HTTPError, UnretriableError):
        assert False, "OkClient.check_response_status_code should not raise error for HTTP 2xx statuses"


def test_get_embed_url():
    ok = OkClient(token="test_construct_url")
    uuid = "ABC123"
    url = ok.get_embed_url(uuid)

    assert "/api/" not in url
    assert "_embedded=1" in url
    assert uuid in url
