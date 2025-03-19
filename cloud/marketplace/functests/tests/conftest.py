import os
from functools import partial

import pytest
import requests
from yc_common import config
from yc_common import ids
from yc_common.clients.identity_v3 import IdentityClient

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
# OAUTH token for yndx-cloud-mkt
from cloud.marketplace.functests.yc_marketplace_functests.fixtures import get_fixtures
from cloud.marketplace.functests.yc_marketplace_functests.utils import DBFixture

TEST_OAUTH_TOKEN = os.getenv("FUNC_TEST_OAUTH_TOKEN")

GOOD_AUTH_VALUE = {"user_account": {"id": os.getenv("FUNC_TEST_USER_ID")}}
BAD_AUTH_VALUE = None


class GauthlingContext:
    def __init__(self, auth: bool = True, authz: bool = True):
        self.auth = auth
        self.authz = authz
        self.url = config.get_value("endpoints.access_service.url")

    def update_auth(self, value):
        if value:
            requests.put("{}/authenticate".format(self.url), json=GOOD_AUTH_VALUE)
        else:
            requests.put("{}/authenticate".format(self.url), json=BAD_AUTH_VALUE)

    def update_authz(self, value):
        if value:
            requests.put("{}/authorize".format(self.url), json=GOOD_AUTH_VALUE)
        else:
            requests.put("{}/authorize".format(self.url), json=BAD_AUTH_VALUE)

    def __enter__(self):
        # self.status = requests.get("{}/gauthling/state".format(self.url)).json()
        self.update_auth(self.auth)
        self.update_authz(self.authz)

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.update_auth(True)
        self.update_authz(True)


@pytest.fixture(autouse=True, scope="session")
def setup_config(request):
    config.load([os.getenv("TEST_CONFIGS_PATH")])


def pytest_addoption(parser):
    parser.addoption("--devel", action="store_true", help="run in development mode")
    parser.addoption("--docker", action="store_true", help="run in docker")


@pytest.fixture(scope="session")
def default_project_id():
    return "00000000-0000-0000-0000-000000000000"


@pytest.fixture(scope="session")
def default_os_product_category():
    return config.get_value("marketplace.default_os_product_category",
                            default="d7600000000000000000")


@pytest.fixture(scope="session")
def default_simple_product_category():
    return config.get_value("marketplace.default_simple_product_category",
                            default="d7610000000000000000")


@pytest.fixture(scope="session")
def default_saas_product_category():
    return config.get_value("marketplace.default_saas_product_category",
                            default="d7620000000000000000")


@pytest.fixture(scope="session")
def default_publisher_category():
    return config.get_value("marketplace.default_publisher_category")


@pytest.fixture(scope="session")
def default_var_category():
    return config.get_value("marketplace.default_var_category")


@pytest.fixture(scope="session")
def default_isv_category():
    return config.get_value("marketplace.default_isv_category")


@pytest.fixture(scope="session")
def iam_token():
    return IdentityClient(config.get_value("endpoints.identity.url"), TEST_OAUTH_TOKEN).get_iam_token()


@pytest.fixture(scope="session")
def marketplace_client(iam_token):
    return MarketplaceClient(config.get_value("endpoints.marketplace.url"), iam_token=iam_token, timeout=20)


@pytest.fixture(scope="session")
def marketplace_private_client(iam_token):
    return MarketplacePrivateClient(config.get_value("endpoints.marketplace_private.url"), iam_token=iam_token,
                                    timeout=20)


@pytest.fixture(scope="session")
def generate_id():
    return partial(ids.generate_id, config.get_value("marketplace.id_prefix"))


@pytest.fixture()
def gauthling_context():
    return lambda *args, **kwargs: GauthlingContext(*args, **kwargs)


@pytest.fixture(scope="function", autouse=True)
def db_fixture(request, setup_config):
    module_name = request.module.__name__[len("__tests__."):]

    try:
        current_fixture = get_fixtures()[module_name][request.function.__name__]
        fixture = DBFixture(current_fixture)
        marketplace_db().query(fixture.render_clean(), commit=True)
        marketplace_db().query(fixture.render_apply(), commit=True)
        yield current_fixture
    except KeyError:
        yield
