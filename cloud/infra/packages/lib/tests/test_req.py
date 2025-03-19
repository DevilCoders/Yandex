from unittest import mock, TestCase
import json
import urllib3
from parameterized import parameterized

from ycinfra import Req


class TestReqClass(TestCase):
    mock_response_text = "Mocked response"
    dummy_url = "https://dummy_url"
    dummy_method = "POST"

    def valid_json_response(self):
        resp = urllib3.response.HTTPResponse
        resp.status = 200
        resp.data = json.dumps(self.mock_response_text).encode("utf-8")
        return resp

    def test_make_request_json_valid(self):
        with mock.patch.object(urllib3.PoolManager, "request") as mocked_request:
            mocked_request.return_value = self.valid_json_response()
            res = Req().make_request_json(
                url=self.dummy_url,
                method=self.dummy_method,
                headers={"header": "Value"},
                data="data"
            )
            self.assertEqual(res, self.mock_response_text)

    @parameterized.expand([
        (
            "wrong headers type",
            dummy_url,
            dummy_method,
            "sting_header",
        ),
        (
            "wrong url type",
            100500,
            dummy_method,
            None,
        ),
        (
            "wrong method",
            dummy_url,
            "FAKE",
            None,
        ),
        (
            "null url",
            None,
            dummy_method,
            None,
        ),
    ])
    def test_make_request_json(self, case_name, url, method, headers):
        response = Req().make_request_json(
            url=url,
            method=method,
            headers=headers
        )
        self.assertIsNone(response)
