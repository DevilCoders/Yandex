""" ApiClient test """

import json
import pytest
import responses
from yc_common.clients.api import ApiClient, ApiError, YcClientProtocolError
from yc_common.models import Model, StringType

BASE_URL = "http://base.url"


class CustomError(Model):
    result = StringType(required=True)
    message = StringType(required=True)


class CustomApiClient(ApiClient):
    def _parse_error(self, response):
        error, _ = self._parse_json_response(response, model=CustomError)
        raise ApiError(response.status_code, response.status_code, error["message"])


@responses.activate
def test_simple_get_ok():
    responses.add(responses.GET, BASE_URL + "/", json={"foo": "bar"})
    client = ApiClient(BASE_URL)
    r = client.get("/")
    assert r == {"foo": "bar"}


@responses.activate
def test_non_json():
    def callback(request):
        assert request.body == "vvv=a&vvv=b&vvv=c"
        assert request.headers["content-type"] == "application/x-www-form-urlencoded"
        body = json.dumps({"foo": "bar"})
        return (200, {}, body)

    responses.add_callback(responses.POST, BASE_URL + "/",
                           callback=callback,
                           content_type="application/json")
    client = ApiClient(BASE_URL)
    client._set_json_requests(False)
    r = client.post("/", {"vvv": ["a", "b", "c"]})
    assert r == {"foo": "bar"}


@responses.activate
def test_non_json_multipart():
    def callback(request):
        assert "multipart/form-data" in request.headers["content-type"]
        assert "name=\"vvv\"; filename=\"test_file\"\r\n\r\nfiledata\r\n" in request.body.decode('utf8')

        body = json.dumps({"foo": "bar"})
        return (200, {}, body)

    responses.add_callback(responses.POST, BASE_URL + "/", callback=callback, content_type="application/json")
    client = ApiClient(BASE_URL)
    client._set_json_requests(False)
    r = client.post("/", {}, files={"vvv": ("test_file", "filedata")})
    assert r == {"foo": "bar"}


@responses.activate
def test_path():
    responses.add(responses.GET, BASE_URL + "/some/path", json={"foo": "bar"})
    client = ApiClient(BASE_URL)
    r = client.get("/some/path")
    assert r == {"foo": "bar"}


@responses.activate
def test_extra_headers():
    EXTRA_HEADERS = {"X-Extra-Hdr": "HeaderValue"}
    responses.add(responses.GET, BASE_URL + "/", json={"foo": "bar"})
    client = ApiClient(BASE_URL, extra_headers=EXTRA_HEADERS)
    r = client.get("/")
    assert r == {"foo": "bar"}
    assert len(responses.calls) == 1
    for hdr in EXTRA_HEADERS:
        assert responses.calls[0].request.headers[hdr] == EXTRA_HEADERS[hdr]


@responses.activate
def test_bad_error():
    responses.add(responses.GET, BASE_URL + "/", json={"foo": "bar"}, status=500)
    client = ApiClient(BASE_URL)
    with pytest.raises(YcClientProtocolError):
        client.get("/")


@responses.activate
def test_good_error():
    responses.add(responses.GET, BASE_URL + "/", json={"code": "XCPT123", "message": "some error description"},
                  status=500)
    client = ApiClient(BASE_URL)
    with pytest.raises(ApiError) as e:
        client.get("/")
    assert e.value.http_code == 500
    assert e.value.code == "XCPT123"
    assert e.value.message == "some error description"


@responses.activate
def test_custom_error():
    responses.add(responses.GET, BASE_URL + "/", json={"result": "Error", "message": "some error description"},
                  status=500)
    client = CustomApiClient(BASE_URL)
    with pytest.raises(ApiError) as e:
        client.get("/")
    assert e.value.http_code == 500
    assert e.value.code == 500
    assert e.value.message == "some error description"


@responses.activate
def test_json_post():
    def callback(request):
        payload = json.loads(request.body)
        assert payload == {"vvv": ["a", "b", "c"]}
        headers = {}
        body = json.dumps({"foo": "bar"})
        return (200, headers, body)

    responses.add_callback(responses.POST, BASE_URL + "/", callback=callback, content_type="application/json")
    client = ApiClient(BASE_URL)
    r = client.post("/", {"vvv": ["a", "b", "c"]})
    assert r == {"foo": "bar"}


@responses.activate
def test_classic_post():
    def callback(request):
        assert request.body == "vvv=a&vvv=b&vvv=c"
        assert request.headers["Content-Type"] == "application/x-www-form-urlencoded"
        headers = {}
        body = json.dumps({"foo": "bar"})
        return (200, headers, body)

    responses.add_callback(responses.POST, BASE_URL + "/", callback=callback, content_type="application/json")
    client = ApiClient(BASE_URL)
    client._set_json_requests(False)
    r = client.post("/", {"vvv": ["a", "b", "c"]})
    assert r == {"foo": "bar"}
