import json
import requests
import pytest
import time
from hamcrest import is_, has_item, has_entry, contains_string, has_key, assert_that, raises, calling
from library.python.testing.pyremock.lib.pyremock import mocked_http_server, MatchRequest, MockResponse, HttpMethod
from random import shuffle
from base_test import BaseTest


class TestMockServer(BaseTest):
    def test_simple(self):
        path = '/api/async/mail/subscriptions'
        request = MatchRequest(method=is_(HttpMethod.GET),
                               path=is_(path))
        mock_response = MockResponse(status=200, body=json.dumps({"result": "ok"}))
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, path)
        response = requests.get(url, timeout=1)

        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok
        assert response.json() == {"result": "ok"}

    def test_multiple_servers(self):
        first_mock_server, first_mock_port = self._start_mock()
        second_mock_server, second_mock_port = self._start_mock()

        first_path = '/complex_move'
        first_url = self._make_url(first_mock_port, first_path)
        first_mock_server.expect(MatchRequest(path=is_(first_path)), MockResponse(body=json.dumps({"result": "ok"})))

        second_path = '/api/async/mail/search'
        second_url = self._make_url(second_mock_port, second_path)
        second_mock_server.expect(MatchRequest(path=is_(second_path)),
                                  MockResponse(body=json.dumps(["mid1", "mid2", "mid3"])))

        first_response = requests.get(first_url, timeout=1)
        assert first_response.status_code == requests.codes.ok
        assert first_response.json() == {"result": "ok"}

        second_response = requests.get(second_url, timeout=1)
        assert second_response.status_code == requests.codes.ok
        assert second_response.json() == ["mid1", "mid2", "mid3"]

        first_mock_server.stop()
        second_mock_server.stop()

        first_mock_server.assert_expectations()
        second_mock_server.assert_expectations()

    def test_response_delay(self):
        mock_response = MockResponse(status=200, body='result ok', delay=1)
        self.mock_server.expect(MatchRequest(path=is_('/path1')), mock_response)
        url = self._make_url(self.mock_port, "/path1")

        start = time.time()
        response = requests.get(url, timeout=5)
        end = time.time()
        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok
        assert end - start > 1

    def test_response_delay_with_dedicated_server(self):
        mock_server, mock_port = self._start_mock()

        mock_response = MockResponse(status=200, body='result ok', delay=1)
        mock_server.expect(MatchRequest(path=is_('/path1')), mock_response)
        url = self._make_url(mock_port, "/path1")

        start = time.time()
        response = requests.get(url, timeout=5)
        end = time.time()
        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok
        assert end - start > 1

        mock_server.stop()

    def test_context_manager(self):
        mock_port = self.pm.get_port()
        with mocked_http_server(mock_port) as mock_server:
            mock_server.expect(MatchRequest(path=is_('/path')))
            url = self._make_url(mock_port, "/path")
            response = requests.get(url, timeout=1)
            self.mock_server.assert_expectations()
            assert response.status_code == requests.codes.ok

    def test_context_manager_exception(self):
        with pytest.raises(RuntimeError):
            with mocked_http_server(self.pm.get_port()):
                raise RuntimeError("another error")

    def test_no_match(self):
        self.mock_server.expect(MatchRequest(path=is_('/path2')))
        url = self._make_url(self.mock_port, "/path1")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.not_found
        assert_that(calling(self.mock_server.assert_expectations), raises(AssertionError))

    def test_post(self):
        self.mock_server.expect(MatchRequest(method=is_(HttpMethod.POST),
                                             path=is_('/path')))
        url = self._make_url(self.mock_port, "/path")
        response = requests.post(url, timeout=1)
        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok

    def test_requests_more_than_expect(self):
        path = '/api/async/mail/subscriptions'
        url = self._make_url(self.mock_port, path)
        self.mock_server.expect(MatchRequest(path=is_(path)), MockResponse(body=json.dumps({"result": "ok"})), 2)

        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.ok
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.ok
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.not_found
        assert_that(calling(self.mock_server.assert_expectations), raises(AssertionError))

    def test_reset(self):
        self.mock_server.expect(MatchRequest(path=is_('/path')))
        self.mock_server.reset()
        url = self._make_url(self.mock_port, "/path")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.not_found
        assert_that(calling(self.mock_server.assert_expectations), raises(AssertionError))

    def test_match_query_params(self):
        query = '/search?user=123456&text=some_text'
        request = MatchRequest(method=is_(HttpMethod.GET),
                               params=has_entry("user", [b"123456"]))
        mock_response = MockResponse(status=200, body=json.dumps({"result": "ok"}))
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, query)
        response = requests.get(url, timeout=1)

        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok
        assert response.json() == {"result": "ok"}

    def test_match_body(self):
        self.mock_server.expect(MatchRequest(method=is_(HttpMethod.POST),
                                             body=contains_string('hello')))
        url = self._make_url(self.mock_port, "/path")
        response = requests.post(url, data='hello mock', timeout=1)
        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok

    def test_match_header(self):
        self.mock_server.expect(MatchRequest(method=is_(HttpMethod.GET),
                                             headers=has_entry("X-Request-Id", has_item(is_('qwerty')))))
        url = self._make_url(self.mock_port, "/path")
        response = requests.get(url, headers={'X-Request-Id': 'qwerty'}, timeout=1)

        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok

    def test_response_status(self):
        mock_response = MockResponse(status=403)
        self.mock_server.expect(MatchRequest(path=is_('/path')), mock_response)
        url = self._make_url(self.mock_port, "/path")
        response = requests.get(url, timeout=1)

        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.forbidden

    def test_response_headers(self):
        mock_response = MockResponse(headers={'Ticket': 'abc123'})
        self.mock_server.expect(MatchRequest(path=is_('/path')), mock_response)
        url = self._make_url(self.mock_port, "/path")
        response = requests.get(url, timeout=1)

        self.mock_server.assert_expectations()
        assert response.status_code == requests.codes.ok
        assert response.headers['Ticket'] == 'abc123'

    def test_complex(self):
        match_request = MatchRequest(params=has_entry("query", has_item(is_(b"first"))))
        mock_response = MockResponse(status=201, headers={'Ticket': 'first_ticket'}, body='first response')
        self.mock_server.expect(match_request, mock_response)

        match_request = MatchRequest(path=contains_string('path'), params=has_key('user'))
        mock_response = MockResponse(status=504, headers={'CustomHeader': 'second_header'}, body='second response')
        self.mock_server.expect(match_request, mock_response)

        match_request = MatchRequest(path=contains_string('third'))
        mock_response = MockResponse(status=400, headers={'Ticket': 'third_ticket'}, body='third response')
        self.mock_server.expect(match_request, mock_response)

        url = self._make_url(self.mock_port, "/path?query=first")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.created
        assert response.headers['Ticket'] == 'first_ticket'
        assert response.text == 'first response'

        url = self._make_url(self.mock_port, "/path?query=second&user=123")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.gateway_timeout
        assert response.headers['CustomHeader'] == 'second_header'
        assert response.text == 'second response'

        url = self._make_url(self.mock_port, "/third_path")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.bad_request
        assert response.headers['Ticket'] == 'third_ticket'
        assert response.text == 'third response'

        self.mock_server.assert_expectations()

    def test_invalid_request(self):
        query = '/search?user=123456&text=some_text'
        request = MatchRequest(method=is_(HttpMethod.GET),
                               params=has_entry("user", has_item(is_(b"some_text"))))
        mock_response = MockResponse(status=200, body="")
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, query)
        response = requests.get(url, timeout=1)

        assert_that(calling(self.mock_server.assert_expectations), raises(AssertionError))
        assert response.status_code == requests.codes.bad_request

    def test_one_route_with_a_different_requests(self):
        request1 = MatchRequest(method=is_(HttpMethod.GET),
                                path=is_("/route"),
                                params=has_entry("user", has_item(is_(b"some_text"))))
        self.mock_server.expect(request1, MockResponse(status=200, body=""))

        request2 = MatchRequest(method=is_(HttpMethod.GET),
                                path=is_("/route"),
                                params=has_entry("id", has_item(is_(b"42"))))
        self.mock_server.expect(request2, MockResponse(status=200, body=""))

        urls = [self._make_url(self.mock_port, query="/route?user=some_text"),
                self._make_url(self.mock_port, query="/route?id=42")]
        shuffle(urls)
        for url in urls:
            assert requests.get(url, timeout=1).status_code == requests.codes.ok
        self.mock_server.assert_expectations()
