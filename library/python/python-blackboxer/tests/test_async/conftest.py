import json
import re
from codecs import open
from os import path

from aioresponses import aioresponses
import pytest
import yatest

FIXTURES_PATH = "library/python/python-blackboxer/tests/fixtures"
pytest_plugins = "aiohttp.pytest_plugin"


@pytest.fixture
def dump_as_json(request):
    def wrapped(f_name, data):
        fname = path.join(request.fspath.dirname, "fixtures", f_name + ".txt")

        with open(fname, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)

    return wrapped


class AsyncHttpMockPlugin(object):
    def pytest_runtest_setup(self, item):
        """
        :param pytest.Item item:
        """
        http_mock = item.get_closest_marker('async_http_mock')
        if not http_mock:
            return

        url = http_mock.kwargs.get("url", re.compile(".*"))
        http_method = http_mock.kwargs.get("http_method", "GET")
        http_code = http_mock.kwargs.get("http_code", 200)
        content_type = http_mock.kwargs.get("content_type", "application/json")
        body = http_mock.kwargs.get("body")

        if not body:
            fname = "%s.txt" % item.name
            fpath = path.join(FIXTURES_PATH, fname)
            real_path = yatest.common.source_path(fpath)
            with open(real_path, encoding="utf-8") as f:
                body = f.read()

        self.aioresp = aioresponses()
        self.aioresp.start()

        for _ in range(10):  # 10 responses, just in case
            self.aioresp.add(
                url, method=http_method, status=http_code, body=body, content_type=content_type
            )

    def pytest_runtest_teardown(self, item):
        http_mock = item.get_closest_marker('async_http_mock')
        if not http_mock:
            return

        self.aioresp.stop()


def pytest_configure(config):
    config.pluginmanager.register(AsyncHttpMockPlugin(), "async_http_mock")
