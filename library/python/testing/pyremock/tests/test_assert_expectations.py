import pytest
import requests
from hamcrest import assert_that, calling, raises, contains_string
from hamcrest.core.base_matcher import BaseMatcher
from library.python.testing.pyremock.lib.pyremock import MatchRequest, MockResponse, assert_expectations
from base_test import BaseTest


class TestAssertExpectations(BaseTest):
    class _RaisingMatcher(BaseMatcher):
        def _matches(self, item):
            raise Exception("Match failed")

        def describe_to(self, description):
            return ""

    def test_exception_in_route_matcher(self):
        request = MatchRequest(method=self._RaisingMatcher())
        mock_response = MockResponse(status=200, body="")
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, "")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.not_found
        assert_that(calling(self.mock_server.assert_expectations), raises(AssertionError))

    def test_exception_in_request_matcher(self):
        request = MatchRequest(body=self._RaisingMatcher())
        mock_response = MockResponse(status=200, body="")
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, "")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.bad_request
        assert_that(calling(self.mock_server.assert_expectations), raises(AssertionError))

    @assert_expectations
    def _test_assert_expectations_decorator_with_failed_expectations(self):
        request = MatchRequest()
        mock_response = MockResponse(status=200, body="")
        self.mock_server.expect(request, mock_response)

    def test_assert_expectations_decorator_with_failed_expectations(self):
        assert_that(calling(self._test_assert_expectations_decorator_with_failed_expectations),
                    raises(AssertionError))

    @assert_expectations
    def test_assert_expectations_decorator_with_successed_expectations(self):
        request = MatchRequest()
        mock_response = MockResponse(status=200, body="")
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, "")
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.ok

    @pytest.mark.parametrize("arg", [0, 1])
    @assert_expectations
    def test_assert_expectations_decorator_with_parametrization(self, arg):
        request = MatchRequest(path=contains_string(str(arg)))
        mock_response = MockResponse(status=200, body="")
        self.mock_server.expect(request, mock_response)

        url = self._make_url(self.mock_port, "/%d" % arg)
        response = requests.get(url, timeout=1)
        assert response.status_code == requests.codes.ok
