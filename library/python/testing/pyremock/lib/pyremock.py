import datetime
import traceback
from collections import namedtuple
from contextlib import contextmanager
from threading import Event, Thread

import tornado.gen
import tornado.httpserver
import tornado.ioloop
import tornado.web
from decorator import decorator
from hamcrest import anything
from hamcrest.core.string_description import StringDescription

from retrying import retry, RetryError


class HttpMethod(object):
    """Enumeration of HTTP methods"""
    GET = 'get'
    POST = 'post'


class MatchRequest(object):
    """Collection of matchers for HTTP request - a matcher for path, a matcher for query params, etc.
    Each request is checked against this matchers using AND logic.
    If all of them match, we consider the request matched and output specified response for it.
    """

    def __init__(self, method=anything(), path=anything(), params=anything(), body=anything(), headers=anything()):
        self.method = method
        self.path = path
        self.params = params
        self.body = body
        self.headers = headers

    def match_route(self, method, path, mismatch_description):
        try:
            return self.method.matches(method) and self.path.matches(path)
        except Exception:
            mismatch_description.append_text("Route matching failed with exception:\r\n")
            mismatch_description.append_text("%s\r\n" % traceback.format_exc())
            return False

    def match(self, params, body, headers, mismatch_description):
        def match_element(matcher, value):
            try:
                if not matcher.matches(value, mismatch_description):
                    mismatch_description.append_text("\r\n")
                    mismatch_description.append_text("expected ")
                    mismatch_description.append_description_of(matcher)
                    mismatch_description.append_text("\r\n")
                    return False
                else:
                    return True
            except Exception:
                mismatch_description.append_text("Request matching failed with exception:\r\n")
                mismatch_description.append_text("%s\r\n" % traceback.format_exc())
                return False

        params_match = match_element(self.params, params)
        body_match = match_element(self.body, body.decode())

        return params_match \
            and body_match \
            and self.headers.matches(headers, mismatch_description)


class MockResponse(object):
    """Represents a typical HTTP-response. Set this for your MatchRequest-object
    to receive specified response.
    """

    def __init__(self, status=200, body='', headers={}, delay=None):
        self.status = status
        self.headers = headers
        self.body = body
        self.delay = delay


class Stub(object):
    def __init__(self, request, response, times):
        self.request = request
        self.response = response
        self.times = 0
        self.expected_times = times

    @property
    def exhausted(self):
        return self.times == self.expected_times


class MockHandler(tornado.web.RequestHandler):
    def initialize(self, stubs, failures):
        self.stubs = stubs
        self.failures = failures

    @tornado.gen.coroutine
    def get(self):
        yield self._handle_request(HttpMethod.GET)

    @tornado.gen.coroutine
    def post(self):
        yield self._handle_request(HttpMethod.POST)

    @tornado.gen.coroutine
    def _output(self, response):
        if response.delay:
            yield tornado.gen.sleep(response.delay)
        self.set_status(response.status)
        for name, value in response.headers.items():
            self.set_header(name, value)
        self.finish(response.body)

    def _match_route(self, routes, mismatch_description):
        descriptions = []
        MatchResult = namedtuple("MatchResult", ["stub", "index", "descriptions"])
        for index, stub in routes:
            try:
                request_mismatch_description = StringDescription()
                if stub.request.match(params=self.request.query_arguments,
                                      body=self.request.body,
                                      headers=self._make_dict(self.request.headers),
                                      mismatch_description=request_mismatch_description):
                    return MatchResult(stub, index, descriptions=[])
                else:
                    descriptions.append(request_mismatch_description)
            except Exception:
                mismatch_description.append_text("\r\n%s" % traceback.print_exc())
        return MatchResult(stub=None, index=-1, descriptions=descriptions)

    @tornado.gen.coroutine
    def _handle_request(self, method):
        mismatch_description = StringDescription()
        routes = [(index, stub) for index, stub in enumerate(self.stubs)
                  if stub.request.match_route(method=method, path=self.request.path,
                                              mismatch_description=mismatch_description)
                  and not stub.exhausted]

        if not routes:
            mismatch_description.append_text("\r\nUnexpected request: method=") \
                .append_description_of(method) \
                .append_text(", path=") \
                .append_description_of(self.request.path)
            self.failures.append(mismatch_description)
            yield self._output(MockResponse(404, "No matching stubs found"))
            return

        stub, index, request_mismatch_descriptions = self._match_route(routes, mismatch_description)

        if stub:
            if stub.exhausted:
                exhaust_descr = StringDescription()
                exhaust_descr.append('Stub is already exhausted')
                request_mismatch_descriptions.append(exhaust_descr)
            else:
                stub.times += 1
                yield self._output(stub.response)
        else:
            mismatch_description.append_text("\r\nUnmatched request: method=") \
                .append_description_of(method) \
                .append_text(", path=") \
                .append_description_of(self.request.path)
            for description in request_mismatch_descriptions:
                mismatch_description.append_text("\r\n")
                mismatch_description.append_text(description)
            yield self._output(MockResponse(400, "Bad request"))

        if str(mismatch_description):
            self.failures.append(mismatch_description)

    def _make_dict(self, headers):
        res = {}
        for key in headers.keys():
            res[key] = headers.get_list(key)
        return res


class MockApplication(tornado.web.Application):
    def __init__(self):
        self.stubs = []
        self.failures = []
        handlers = [
            (r'/.*', MockHandler, dict(stubs=self.stubs, failures=self.failures)),
        ]
        super(MockApplication, self).__init__(handlers)

    def expect(self, request, response, times):
        self.stubs.append(Stub(request, response, times))

    def reset(self):
        self.stubs[:] = []
        self.failures[:] = []

    def pending(self):
        return [stub for stub in self.stubs if not stub.exhausted]

    def failed(self):
        return self.failures

    @staticmethod
    def _append_stub_not_called_message(stub, mismatch_description):
        mismatch_description.append_text("\r\nExpected request didn't called properly, method=") \
            .append_description_of(stub.request.method) \
            .append_text(", path=") \
            .append_description_of(stub.request.path) \
            .append_text(", params=") \
            .append_description_of(stub.request.params) \
            .append_text(", body=") \
            .append_description_of(stub.request.body) \
            .append_text(", called %d times (expected %d)" % (stub.times, stub.expected_times))

    def expectations_fail_description(self):
        mismatch_description = StringDescription()
        mismatch_description.append_text("\r\n".join(str(f) for f in self.failed()))
        for stub in self.pending():
            if stub.expected_times is not None:
                self._append_stub_not_called_message(stub, mismatch_description)
        return mismatch_description

    def assert_expectations(self):
        errors = str(self.expectations_fail_description())
        if errors:
            raise AssertionError(errors)


class MockHttpServer(object):
    """Mock server that implements HTTP and outputs specified response based on matched request patterns.
    Listens to given port in a separate thread.
    If no stub is matched, outputs '404 Not Found'.
    """

    def __init__(self, port, host='localhost'):
        """Initializes server with given port and internal handlers"""
        self.application = MockApplication()
        self.port = port
        self.host = host
        self.ready_event = Event()

    def _inner_start(self):
        self.ioloop.make_current()
        self.server.listen(self.port)
        self.ready_event.set()
        self.ioloop.start()

    @property
    def url(self):
        return 'http://{self.host}:{self.port}'.format(self=self)

    def start(self, timeout=5):
        """Start listening for connections and handling requests"""
        self.ready_event.clear()
        self.ioloop = tornado.ioloop.IOLoop(make_current=False)
        self.server = tornado.httpserver.HTTPServer(self.application)
        self.thread = Thread(target=self._inner_start)
        self.thread.start()
        if not self.ready_event.wait(timeout=timeout):
            raise RuntimeError('Could not wait for MockHttpServer to be started')

    def stop(self):
        """Stop listening and responding"""
        self.server.stop()
        self.ioloop.add_callback(self.ioloop.stop)
        self.thread.join()

    def expect(self, request, response=MockResponse(), times=1):
        """Add response stub for request pattern"""
        self.application.expect(request, response, times)

    def reset(self):
        """Delete all stubs"""
        self.application.reset()

    def pending(self):
        """Check if application still waits for requests"""
        return self.application.pending()

    def failed(self):
        """Check if application has already got an unexpected request"""
        return self.application.failed()

    def wait_for(self, min_timeout=datetime.timedelta(milliseconds=10), max_timeout=datetime.timedelta(seconds=1)):
        """
        Wait until all pending requests are satisfied, exits early on any failure.
        Call `self.assert_expectations()` after wait to check your conditions.
        """

        @retry(
            retry_on_result=lambda x: not x,
            wait_exponential_multiplier=1000 * min_timeout.total_seconds(),
            stop_max_delay=1000 * max_timeout.total_seconds(),
        )
        def is_ready():
            return self.failed() or not self.pending()
        try:
            return is_ready()
        except RetryError:
            return False

    def expectations_fail_description(self):
        """
        :return: Expectations mismatch description
                 If there is no failed expectations, description is empty
        """
        return self.application.expectations_fail_description()

    def assert_expectations(self):
        """Assert that expectations are met"""
        self.application.assert_expectations()


@contextmanager
def mocked_http_server(port, host='localhost'):
    """Context manager for MockHttpServer"""
    try:
        server = MockHttpServer(port, host=host)
        server.start()
        yield server
    finally:
        server.stop()


@decorator
def assert_expectations(func, self, *args, **kwargs):
    mocks = filter(lambda attr: isinstance(attr, MockHttpServer),
                   map(lambda attr: getattr(self, attr), dir(self)))
    assertion_message = None

    try:
        func(self, *args, **kwargs)
    except AssertionError:
        assertion_message = traceback.format_exc()

    mismatch_description = StringDescription()
    if assertion_message:
        mismatch_description.append_text(assertion_message)

    fails = list(filter(
        lambda desc: len(desc) > 0,
        map(lambda mock: str(mock.expectations_fail_description()), mocks)
    ))
    if fails:
        mismatch_description.append_text("\r\n")
        mismatch_description.append_text("\r\n".join(fails))

    error_string = str(mismatch_description)
    if error_string:
        raise AssertionError(error_string)
