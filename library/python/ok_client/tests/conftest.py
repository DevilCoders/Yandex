import json
import pytest
import requests
import requests_mock
import typeguard
import uuid
import re

from library.python.ok_client import OkClient, CreateApprovementRequest
from library.python import resource


@pytest.fixture(scope="session")
def existing_approvements():
    return [
        json.loads(resource.find(path))
        for path in resource.iterkeys(prefix="ok_approvement/")
    ]


@pytest.fixture(scope="session")
def post_approvement_callback(existing_approvements):

    def callback(request: requests.Request, context):

        request_json = request.json()

        try:
            typeguard.check_type("request.json", CreateApprovementRequest, request_json)
        except TypeError:
            context.status_code = requests.status_codes.codes.bad_request
            result = {
                'errors': {
                    'object_id': [{'code': 'required'}],
                    'text': [{'code': 'required'}],
                    'stages': [{'code': 'required'}],
                },
            }

            return result

        for key in {"object_id", "text", "stages"}:
            if key not in request_json:
                context.status_code = requests.status_codes.codes.bad_request
                result = {
                    'errors': {
                        'object_id': [{'code': 'required'}],
                        'text': [{'code': 'required'}],
                        'stages': [{'code': 'required'}],
                    },
                }

                return result

        for existing_approvement_item in existing_approvements:

            if request_json["object_id"] != existing_approvement_item["object_id"]:
                continue

            context.status_code = requests.status_codes.codes.bad_request
            result = {
                "error": [
                    {
                        "code": "already_exists",
                        "message": "already_exists",
                        "params": {
                            "uuid": existing_approvement_item["uuid"],
                            "uid": "something",
                        },
                    },
                ],
            }

            return result

        return {
            **request_json,
            **{
                "id": 9999,
                "uuid": str(uuid.uuid4()),
                "stats": None,
                "tracker_queue": {
                    "name": request_json["object_id"].split("-", 1)[0],
                    "has_triggers": True,
                },
                "user_roles": {
                    "responsible": True,
                    "approver": True,
                },
                "actions": {
                    "approve": True,
                    "reject": True,
                    "suspend": True,
                    "resume": True,
                    "close": True,
                },
            },
        }

    return callback


@pytest.fixture(scope="session")
def get_approvement_callback(existing_approvements):

    def callback(request: requests.Request, context):

        uuid = request.url.strip("/").rsplit("/", 1)[-1]

        for approvement in existing_approvements:

            if approvement["uuid"] != uuid:
                continue

            context.status_code = requests.status_codes.codes.ok

            return approvement

        context.status_code = requests.status_codes.codes.not_found

        return {}

    return callback


@pytest.fixture(scope="session")
def mocked_ok_base_url():
    return "http://ok.client.test.y-t.ru/"


@pytest.fixture(scope="session")
def mocked_ok_client(
    mocked_ok_base_url,
    post_approvement_callback,
    get_approvement_callback,
):

    with requests_mock.Mocker() as m:

        client = OkClient(token="111", base_url=mocked_ok_base_url)

        m.register_uri(
            "POST",
            client.construct_url(client.URL_PATH_APPROVEMENTS),
            json=post_approvement_callback,
        )

        approvement_url_pattern = client.construct_url(r'approvements/[a-z\-0-9]+')
        approvement_url_matcher = re.compile(approvement_url_pattern)

        m.register_uri(
            "GET",
            approvement_url_matcher,
            json=get_approvement_callback,
        )

        yield client


@pytest.fixture
def create_approvement_successful_response_dict():
    return json.loads(resource.find("ok_approvement/d7bd8796-4835-4fb8-a9ab-20a7232e99f3.json"))


@pytest.fixture
def create_approvement_successful_response(create_approvement_successful_response_dict):
    result = requests.Response()

    result.code = "OK"
    result.status_code = 200
    result._content = json.dumps(create_approvement_successful_response_dict).encode("utf-8")

    return result
