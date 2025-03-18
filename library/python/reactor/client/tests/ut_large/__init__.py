import os

from reactor_client.reactor_api import ReactorAPIClientV1, RetryPolicy
from library.python.reactor.client.tests.helpers.utils import get_current_datetime_string, generate_random_string


TEST_USER = "robot-reactor"
REACTOR_TEST_PROJECT_PATH = "/reactor/Project"

namespace_for_tests = "/system/tests/API client"
reactor_url = "https://test.reactor.yandex-team.ru"


def get_path_for_tests(test_name):
    return os.path.join(namespace_for_tests, "test__" + get_current_datetime_string() + "__" + generate_random_string(16) + "__" + test_name)


def get_oauth_token():
    if "OAUTH_TOKEN" in os.environ:
        return os.environ["OAUTH_TOKEN"]
    else:
        home = os.path.expanduser("~")
        path_to_oauth_file = os.path.join(home, ".oauth", TEST_USER)

        with open(path_to_oauth_file) as f:
            line = f.readlines()[0]
            return line.rstrip()


def reactor_client():
    return ReactorAPIClientV1(reactor_url, get_oauth_token(), retry_policy=RetryPolicy())
