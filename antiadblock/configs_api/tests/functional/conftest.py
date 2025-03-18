import os
import re
import json
from collections import namedtuple
from urlparse import urlparse

import pytest
import requests
from requests import Response
from hamcrest import has_entries, is_, any_of, only_contains

from wsgi_intercept.interceptor import RequestsInterceptor
from library.python import resource
import yatest.common
import yatest.common.network

from antiadblock.postgres_local.migrator import FileBasedMigrator
from antiadblock.postgres_local.postgres import Config, PostgreSQL, State

from antiadblock.configs_api.lib.api_version import CURRENT_API_VERSION, CURRENT_INTERNAL_API_VERSION
from antiadblock.configs_api.lib.auth.webmaster import WebmasterAPI
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.utils import URLBuilder
from antiadblock.configs_api.lib.const import PARTNER_TOKEN_HEADER_NAME, DECRYPT_TOKEN_HEADER_NAME, SECRET_DECRYPT_CRYPROX_TOKEN, DECRYPT_RESPONSE_HEADER_NAME
from antiadblock.configs_api.lib.internal_api import TURBO_REDIRECT_SERVICE_ID
from antiadblock.configs_api.lib.sonar.sonar import SonarClient


from tvmauth.mock import MockedTvmClient

from tvmauth import (
    BlackboxEnv,
)


VALID_CONFIG_DATA = has_entries(PARTNER_TOKENS=only_contains(is_(basestring)),
                                CRYPT_SECRET_KEY=is_(basestring),
                                PROXY_URL_RE=is_(list))

VALID_CONFIG = has_entries(id=is_(int),
                           data=VALID_CONFIG_DATA,
                           comment=is_(basestring),
                           statuses=only_contains(any_of(
                               has_entries(status="active"),
                               has_entries(status="test"),
                               has_entries(status="approved"),
                           )),
                           label_id=is_(basestring),
                           parent_label_id=is_(basestring),
                           exp_id=any_of(is_(basestring), is_(None)),
                           creator_id=any_of(is_(int), is_(None)),
                           parent_id=any_of(is_(int), is_(None)))

AAB_ADMIN_TEST_TVM_CLIENT_ID = 2000627  # Adminka qloud stands

ADMIN_USER_ID = 1
USER2_ID = 2
USER3_ID = 3
USER3_ANOTHER_ID = 4

ADMIN_SESSION_ID = "admin_session_id"
USER2_SESSION_ID = "user2_session_id"
USER3_SESSION_ID = "user3_session_id"
UNKNOWN_SESSION_ID = "some_unknown_session_id"
ADMIN_LOGIN = "anatoliy"
USER2_LOGIN = "vasya"
USER3_LOGIN = "petya"
USER3_ANOTHER_LOGIN = "another-petya"
WM_SERVICE_DOMAIN = "musorok.ru"

# The public key pair to the default private key
DEFAULT_PUBLIC_KEY = resource.find("keys/default.pub")
TVM_KEYS = resource.find("keys/test_tvm.key")

# default autoredirect services
AUTOREDIRECT_SERVICES_DEFAULT_CONFIG = {
    'aabturbo-gq': {'domain': 'aabturbo.gq', 'urls': ['http://aabturbo.gq/wp-includes/js/wp-embed.min.js']},
    'aabturbo2-gq': {'domain': 'aabturbo2.gq', 'urls': ['http://aabturbo2.gq/wp-includes/js/wp-embed.min.js']},
}


def _s(url):
    """
    Explicitly stringify url to prevent https://github.com/pupssman/urlbuilder/issues/1 from firing

    FIXME: whyyyyyy
    """
    return '%s' % url


class Session(object):
    """
    Explicitly stringify url on requests methods to prevent https://github.com/pupssman/urlbuilder/issues/1 from firing
    """
    def __init__(self, session_id):
        self._session = requests.session()
        self._session.cookies.update(dict(Session_id=session_id))

    def __getattr__(self, name):
        if name in ["get", "post", "put", "patch", "options", "head", "delete"]:
            def method(url, *args, **kvargs):
                return getattr(self._session, name)(_s(url), *args, **kvargs)
            return method

        if name == "request":
            def request(method, url, *args, **kvargs):
                return self._session.request(method, _s(url), *args, **kvargs)
            return request

        return getattr(self._session, name)


@pytest.fixture(scope='session')
def tickets():
    Tickets = namedtuple('Tickets', ('ADMIN_USER_TICKET', 'USER2_TICKET', 'USER3_TICKET', 'IDM_SERVICE_TICKET', 'CRYPROX_SERVICE_TICKET', 'MONRELY_TICKET'))

    # tickets by 'tvmknife unittest user -e prod -u {user_id}'
    _base_path = yatest.common.source_path('antiadblock/configs_api/tests/functional/tickets')
    with open(os.path.join(_base_path, 'admin_user_ticket.tik')) as ticket:
        admin_user_ticket = ticket.read()

    with open(os.path.join(_base_path, 'user2_ticket.tik')) as ticket:
        user2_ticket = ticket.read()

    with open(os.path.join(_base_path, 'user3_ticket.tik')) as ticket:
        user3_ticket = ticket.read()

    with open(os.path.join(_base_path, 'idm_ticket.tik')) as ticket:
        idm_service_ticket = ticket.read()

    # service ticket for internal_api
    with open(os.path.join(_base_path, 'cryprox_ticket.tik')) as ticket:
        cryprox_service_ticket = ticket.read()

    # service ticket for monrely scheduler
    with open(os.path.join(_base_path, 'monrely.tik')) as ticket:
        monrely_ticket = ticket.read()

    return Tickets(admin_user_ticket, user2_ticket, user3_ticket, idm_service_ticket, cryprox_service_ticket, monrely_ticket)


@pytest.fixture(scope='session')
def configs_app(postgresql_database):

    from antiadblock.configs_api.lib.app import app as configs_app
    from antiadblock.configs_api.lib.app import init_app
    # database run on random port, so we know uri only in runtime
    configs_app.config['SQLALCHEMY_DATABASE_URI'] = postgresql_database._get_sqlalchemy_connection_url()
    init_app()
    return configs_app


@pytest.yield_fixture(scope='function')
def app(postgresql_database, configs_app):
    """
    NB: you HAVE to use `_s` function with this fixture or 'session' fixture

    See https://github.com/pupssman/urlbuilder/issues/1
    """
    with RequestsInterceptor(lambda: configs_app, host='neverevertherewillbeahostlikethis.com', port=80) as url:
        yield URLBuilder(url)


@pytest.yield_fixture(scope='function')
def valid_app_urls(service, session, api, configs_app):
    """
    fixture returns list of valid urls. It substitutes flask rule placeholder with some valid values

    N.B.: If you write new control with new placeholder you should add it to this dict

    :return:
    """

    substitute_map = {
        "<string:service_id>": service['id'],
        "<string:metric_handler_name>": "http_codes",
        "<int:config_id>": session.get(api["label"][service['id']]["config"]["active"]).json()['id'],
        "<int:result_id>": 11,
        "<int:profile_id>": 12,
        "<int:id>": 13,
        "<string:request_id>": "1525eff75d988ef-{cnt}",
        "<string:log_type>": "bs-dsp-log",
        "<string:label_id>": "ROOT",
    }

    def substitute_placeholders(rule):
        s = rule
        for key, value in substitute_map.iteritems():
            s = s.replace(key, str(value))
        return s

    def to_relative(absolute_url):
        """
        remove leading slash
        :param absolute_url:
        :return:
        """
        return absolute_url[1:]

    # checks only rules having valid prefix
    control_rules = filter(lambda r: r.rule.startswith('/' + CURRENT_API_VERSION) or r.rule.startswith('/' + CURRENT_INTERNAL_API_VERSION), configs_app.url_map.iter_rules())

    def first_method(rule):
        """
        returns first lowercased method that is not 'HEAD' or 'OPTIONS'. Fails if no such method
        :param rule:
        :return:
        """
        return map(lambda m: m.lower(),
                   filter(lambda m: m not in ["HEAD", "OPTIONS"], rule.methods))[0]

    result = [(to_relative(substitute_placeholders(r.rule)), first_method(r)) for r in control_rules]

    def filter_guest_permitted_methods(methods):
        guest_methods = [
            (CURRENT_API_VERSION + '/auth/permissions/global', "get"),
            (CURRENT_API_VERSION + '/auth/permissions/service/.*', "get"),
            (CURRENT_API_VERSION + '/services', "get"),
            (CURRENT_API_VERSION + '/labels', "get"),
            (CURRENT_API_VERSION + '/search', "get"),
            (CURRENT_API_VERSION + '/get_all_checks', "get"),
        ]
        return filter(lambda (rule, method): not any(map(lambda (filter_rule, filter_method): re.compile(filter_rule).match(rule) and filter_method == method, guest_methods)), methods)

    return filter_guest_permitted_methods(result)


@pytest.fixture(scope='function')
def blackbox_client_stub(insure_db_schema, tickets):

    class BlackboxStub(object):

        def get_user_ticket(self, session_id, *_):
            sessions = {
                ADMIN_SESSION_ID: tickets.ADMIN_USER_TICKET,
                USER2_SESSION_ID: tickets.USER2_TICKET,
                USER3_SESSION_ID: tickets.USER3_TICKET,
            }
            return sessions.get(session_id)

        def get_user_logins(self, *_):
            return dict([(ADMIN_USER_ID, ADMIN_LOGIN),
                         (USER2_ID, USER2_LOGIN),
                         (USER3_ID, USER3_LOGIN)])

        # yandex login in which " - "replaced by "." considered the same. And vice versa
        def get_user_ids(self, logins):
            return {k: v for k, v in dict([(ADMIN_LOGIN, ADMIN_USER_ID),
                                           (USER2_LOGIN, USER2_ID),
                                           (USER3_LOGIN, USER3_ID),
                                           (USER3_ANOTHER_LOGIN.replace('-', '.'), USER3_ANOTHER_ID)]).items() if k in logins or k.replace('.', '-') in logins}

    CONTEXT.blackbox = BlackboxStub()


@pytest.fixture(scope="function")
def argus_client_stub(insure_db_schema):

    class ArgusClientStub(object):
        def __init__(self):
            self.sandbox_id = 0

        def run_argus_task(self, profile, argus_run_id, argus_resource_id=None):
            if not isinstance(profile, dict):
                raise TypeError("Invalid type of profile")

            if not isinstance(profile["cookies"], basestring):
                raise TypeError("Invalid type of cookies")

            url_settings = profile["url_settings"]
            if not isinstance(url_settings, list):
                raise TypeError("Invalid type of url_settings")

            for item in url_settings:
                if not isinstance(item, dict):
                    raise TypeError("Invalid item[{}] type of url_settings".format(url_settings.index(item)))

                if "url" not in item or "selectors" not in item:
                    raise ValueError("Keys url or selectors are not exists. item[{}]".format(url_settings.index(item)))

                if not isinstance(item["selectors"], list):
                    raise TypeError("Invalid type of selectors list")

            self.sandbox_id += 1
            return self.sandbox_id

        def get_logs_list_from_s3(self, *args, **kwargs):
            return [
                {
                    "dspid": "10",
                    "eventid": "143715039",
                    "iso_eventtime": "2020-05-13 09:41:13",
                    "log_id": "2322624187",
                    "pageid": "330627",
                    "producttype": "ZERO_PRODUCT",
                    "timestamp": "1589352073"
                },
                {
                    "dspid": "1",
                    "eventid": "423133701",
                    "iso_eventtime": "2020-05-13 09:41:14",
                    "log_id": "2322624187",
                    "pageid": "330627",
                    "producttype": "direct",
                    "timestamp": "1589352074"
                },
                {
                    "dspid": "10",
                    "eventid": "930245768",
                    "iso_eventtime": "2020-05-13 09:41:15",
                    "log_id": "2322624187",
                    "pageid": "330627",
                    "producttype": "ZERO_PRODUCT",
                    "timestamp": "1589352075"
                },
                {
                    "dspid": "1",
                    "eventid": "423133701",
                    "iso_eventtime": "2020-05-13 09:41:14",
                    "log_id": "2322624187",
                    "pageid": "330627",
                    "producttype": "direct",
                    "timestamp": "1589352110"
                }
            ]

    CONTEXT.argus_client = ArgusClientStub()


PARTNER_RULES = [{
    'added': '2019-08-28T14:16:26.672303',
    'list_url': 'https://dl.opera.com/download/get/?adblocker=adlist&country=ru',
    'raw_rule': '@@||ads.adfox.ru/*/getBulk$xmlhttprequest,domain=widgets.kinopoisk.ru|www.kinopoisk.ru',
    'is_partner_rule': True,
}]


class YtClientStub(object):

    def __init__(self):
        self.config = dict(proxy=dict())

    def select_rows(self, query_string):
        if 'sonar_rules' in query_string:
            return PARTNER_RULES
        elif 'adblock_sources' in query_string:
            tags = query_string.split('"')[1::2]
            sources = [{
                'list_url': 'https://dl.opera.com/download/get/?adblocker=adlist&country=ru',
                'adblocker': 'Opera',
                'tags': ['Opera', 'desktop']
            }, {
                'list_url': 'https://easylist-downloads.adblockplus.org/advblock+cssfixes.txt',
                'adblocker': 'Ublock',
                'tags': ['Ublock', 'desktop']
            }]
            return list(filter(lambda s: all(tag in s['tags'] for tag in tags), sources))
        else:
            return [{
                'added': '2019-08-07T22:09:37.978809',
                'list_url': 'https://easylist-downloads.adblockplus.org/advblock+cssfixes.txt',
                'raw_rule': '@@||ads.adfox.ru/*/getBulk$xmlhttprequest,domain=www.kinopoisk.ru',
                'is_partner_rule': False,
            }]


class StartrekIssueStub(object):
    tags = None
    key = None

    def __getitem__(self, _):
        return self.key

    def __init__(self, key, tags):
        self.key = key
        self.tags = tags
        self.summary = 'Ticket summary'

    def update(self, *args, **kwargs):
        pass


class StartrekClientStub(object):
    components_hash = {
        'auto.ru': [StartrekIssueStub('123', ['negative_trend'])],
        'yandex_news': [],
        'yandex.startrek': [],
        'yandex.startrek.trend': [StartrekIssueStub('456', ['negative_trend', 'device:desktop'])],
        'yandex.startrek.trend.2': [StartrekIssueStub('789', ['negative_trend', 'device:desktop'])],
        'yandex.startrek.trend.3': [StartrekIssueStub('101112', ['negative_trend', 'device:desktop'])],
    }

    def __getitem__(self, _):
        return self

    @property
    def queues(self):
        return self

    @property
    def issues(self):
        return self

    @property
    def components(self):
        return [{
            'name': 'auto.ru',
            'id': 'auto.ru'
        }, {
            'id': 'yandex_news',
            'name': 'yandex_news'
        }, {
            'id': 'yandex.startrek',
            'name': 'yandex.startrek'
        }, {
            'id': 'yandex.startrek.trend',
            'name': 'yandex.startrek.trend'
        }, {
            'id': 'yandex.startrek.trend.2',
            'name': 'yandex.startrek.trend.2'
        }]

    def find(self, filter):
        return self.components_hash[filter['components'][0]]

    def create(self, *args, **kwargs):
        return StartrekIssueStub('123', ['negative_trend'])


class SonarClientStub(SonarClient):

    def get_partner_rules(self, service_id):
        if service_id != 'test.service':
            return PARTNER_RULES
        return [
            {
                'added': '2019-08-28T14:16:26.672303',
                'list_url': 'https://dl.opera.com/download/get/?adblocker=adlist&country=ru',
                'raw_rule': '''yandex.ru#%#AG_removeCookie('/blcrm|bltsr|^specific\\$|jsOmqPGh/''',
                'is_partner_rule': True,
            },
            {
                'added': '2019-08-28T14:16:26.672303',
                'list_url': 'https://dl.opera.com/download/get/?adblocker=adlist&country=ru',
                'raw_rule': 'yandex.ua##+js(cookie-remover, /^bltsr$|^JPIqApiY$|^substantial$/',
                'is_partner_rule': True,
            }
        ]


@pytest.fixture(scope="function")
def sonar_client_stub(insure_db_schema):
    CONTEXT.sonar_client = SonarClientStub(oauth_token="", yt_client=YtClientStub())


@pytest.fixture(scope="function")
def startrek_client_stub(insure_db_schema):
    CONTEXT.startrek_client = StartrekClientStub()


@pytest.fixture(scope="function")
def abc_function_stub(insure_db_schema):
    CONTEXT.get_abc_duty = lambda _: ('george', 'another_test_duty')


@pytest.fixture(scope='function')
def tvm_client_stub(insure_db_schema):
    mockedTvmClient = MockedTvmClient(self_tvm_id=AAB_ADMIN_TEST_TVM_CLIENT_ID, bb_env=BlackboxEnv.Prod)
    CONTEXT.tvm = mockedTvmClient


@pytest.fixture(scope='function')
def webmaster_client_stub(insure_db_schema, tickets):

    class WebmasterAPISpy(WebmasterAPI):
        def __init__(self, *args, **kvargs):
            super(WebmasterAPISpy, self).__init__(*args, **kvargs)

        # for authentification
        def _retrieve_domains_response(self, user_ticket, _):
            verified_hosts = {
                tickets.USER2_TICKET: dict(verifiedHosts=[
                    dict(hostId='https:{}:443'.format(WM_SERVICE_DOMAIN), verifiedAt='2018-03-23T19:15:58.453Z')]),
            }
            return verified_hosts.get(user_ticket, dict(verifiedHosts=[]))

    CONTEXT.webmaster = WebmasterAPISpy(CONTEXT, "http://some_url_never_gonna_call")


class CryproxClientMock(object):
    crypted_links = {'/so12me/cryp34ted/url/lulz': 'http://an.yandex.ru/decrypted-lulz',
                     'auto.ru/so12me/cryp34ted/url/lulz': 'http://an.yandex.ru/decrypted-lulz',
                     '//auto.ru/so12me/cryp34ted/url/lulz': 'http://an.yandex.ru/decrypted-lulz',
                     'https://auto.ru/so12me/cryp34ted/url/lulz': 'http://an.yandex.ru/decrypted-lulz',
                     '/wo12op/with34args/huh/?o=mg&fc=thsh': 'http://awaps.yandex.net/billy/like.skittles?o=mg&fc=thsh'}
    not_crypted_links = {'/not/crypted/url.js': '/not/crypted/url.js'}
    forbidden_link = '/forbidden/url'

    def get(self, url, **kw):
        parsed_url = urlparse(url)
        path = parsed_url.path
        qargs = parsed_url.query
        headers = kw.get('headers', {})

        response = Response()

        if PARTNER_TOKEN_HEADER_NAME not in headers or path == CryproxClientMock.forbidden_link:
            response.status_code = 403
            response._content = 'Forbidden'
            return response

        if (DECRYPT_TOKEN_HEADER_NAME, SECRET_DECRYPT_CRYPROX_TOKEN) in headers.items() and ('host', service_fixture_configuration['domain']) in headers.items():
            decrypt_stub_dict = CryproxClientMock.crypted_links.copy()
            decrypt_stub_dict.update(CryproxClientMock.not_crypted_links)
            resp = decrypt_stub_dict.get(path + '?{}'.format(qargs) if qargs else path, None)

            if resp is not None:
                response.status_code = 200
                response._content = 'Ok'
                response.headers = {DECRYPT_RESPONSE_HEADER_NAME: resp}

                return response
        response.status_code = 404
        response._content = 'Not found from cryprox'

        return response


@pytest.fixture(scope='function')
def cryprox_client_stub(insure_db_schema):
    CONTEXT.cryprox = CryproxClientMock()


@pytest.fixture(scope='function')
def api(app, cryprox_client_stub):
    return app[CURRENT_API_VERSION]


@pytest.fixture(scope='function')
def internal_api(app):
    """
    NB: you HAVE to use `_s` function with this fixture or 'session' fixture

    See https://github.com/pupssman/urlbuilder/issues/1
    """
    return app[CURRENT_INTERNAL_API_VERSION]


@pytest.fixture(scope='function')
def transactional(clean_database, blackbox_client_stub, tvm_client_stub, webmaster_client_stub, grant_permissions,
                  argus_client_stub, root_label, sonar_client_stub, startrek_client_stub, abc_function_stub):
    grant_permissions(ADMIN_LOGIN, "*", "admin")
    """
    ensures database has no data and user permissions are initialized
    :param transactional:
    :param init_user_permissions:
    :return:
    """


@pytest.fixture(scope='function')
def insure_db_schema(app, session):
    session.get(app.ping)


@pytest.fixture(scope='function')
def grant_permissions(api, session, tickets):
    def grant(login, node, role):
        if node in ["*", "all"]:
            role = json.dumps({"project": node, "role": role})
            path = "/project/{0}/role/{1}/".format(node, role)
        else:
            role = json.dumps({"project": "other", "service": node, "role": role})
            path = "/project/other/service/{0}/role/{1}/".format(node, role)
        response = session.post(api["idm"]["add-role"][""],
                                data=dict(login=login,
                                          role=role,
                                          path=path,
                                          fields=json.dumps({"passport-login": login})),
                                headers={'X-Ya-Service-Ticket': tickets.IDM_SERVICE_TICKET,
                                         'Content-Type': 'application/x-www-form-urlencoded'})
        assert response.status_code == 200
        assert response.json()["code"] == 0
    return grant


@pytest.yield_fixture(scope='session')
def postgresql_database():
    with yatest.common.network.PortManager() as pm:
        postgresql = None
        try:
            psql_config = Config(port=pm.get_port(5432),
                                 keep_work_dir=False,
                                 username='antiadb',
                                 password='postgres',
                                 dbname='configs')
            migrator = FileBasedMigrator(yatest.common.source_path('antiadblock/configs_api/migrations/migrations'))

            postgresql = PostgreSQL(psql_config, migrator)
            postgresql.run()
            postgresql.ensure_state(State.RUNNING)

            yield postgresql
        finally:
            if postgresql is not None:
                postgresql.shutdown()


@pytest.fixture(scope='function')
def clean_database(postgresql_database, insure_db_schema):
    def truncate(db_conn):
        db_conn.execute("DELETE FROM sbs_results")
        db_conn.execute("DELETE FROM sbs_runs")
        db_conn.execute("DELETE FROM sbs_profiles")
        db_conn.execute("DELETE FROM audit_log")
        db_conn.execute("DELETE FROM config_statuses")
        db_conn.execute("DELETE FROM configs")
        db_conn.execute("DELETE FROM service_comments")
        db_conn.execute("DELETE FROM checks_in_progress")
        db_conn.execute("DELETE FROM service_checks")
        db_conn.execute("DELETE FROM services")
        db_conn.execute("DELETE FROM permissions")
        db_conn.execute("DELETE FROM user_logins")
        db_conn.execute("DELETE FROM autoredirect_services")

    postgresql_database.exec_transaction(truncate)


@pytest.yield_fixture(scope='function')
def session():
    yield Session(ADMIN_SESSION_ID)


service_fixture_configuration = dict(service_id="auto.ru",
                                     name="autoru",
                                     domain="auto.ru")


@pytest.fixture(scope="function")
def service(api, session):
    response = session.post(api['service'], json=service_fixture_configuration)
    assert response.status_code == 201
    return response.json()


@pytest.fixture(scope="function")
def service_by_webmaster(api, session):
    # see conftest.webmaster_client_stub
    r = session.post(api['service'], json=dict(service_id=WM_SERVICE_DOMAIN,
                                               name="musorok",
                                               domain=WM_SERVICE_DOMAIN))

    assert r.status_code == 201
    return r.json()


@pytest.fixture(scope="function")
def argus_service_data(postgresql_database, insure_db_schema, service):
    service_id = service["id"]

    def add_data(db_conn):
        query = """INSERT INTO sbs_profiles (service_id, "date", url_settings, tag, is_archived) VALUES
        ('{service_id}', '2019-08-09', '[{{"url": "https://ya.ru"}}, {{"url": "https://yapor.vu", "selectors": []}}]'::jsonb, 'argus_test', FALSE),
        ('{service_id}', '2229-10-10', '[{{"url": "https://vk.ru", "selectors": ["some_select"]}}]'::jsonb, 'default', FALSE)""".format(service_id=service_id)
        db_conn.execute(query)

    postgresql_database.exec_transaction(add_data)


@pytest.fixture(scope="function")
def webmaster_service_data(api, internal_api, session, insure_db_schema, tickets):

    r = session.post(api['service'], json=dict(service_id=TURBO_REDIRECT_SERVICE_ID,
                                               name=TURBO_REDIRECT_SERVICE_ID,
                                               domain='turbo.local'))

    assert r.status_code == 201

    response = session.post(internal_api["redirect_data"], json=AUTOREDIRECT_SERVICES_DEFAULT_CONFIG,
                            headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert response.status_code == 201

    return r.json()


@pytest.fixture(scope="function")
def root_label(postgresql_database, insure_db_schema):

    def add_root(db_conn):
        query = """INSERT INTO configs (comment, data, created, label_id, parent_label_id) VALUES ('Create ROOT on migration', '{}'::jsonb, LOCALTIMESTAMP, 'ROOT', NULL);
                   INSERT INTO audit_log (date, action, params) VALUES (LOCALTIMESTAMP, 'parent_config_create', '{"config_comment": "Create ROOT on migration"}'::jsonb);
                   INSERT INTO config_statuses VALUES (currval('configs_id_seq'), 'active'), (currval('configs_id_seq'), 'test'), (currval('configs_id_seq'), 'approved');"""
        db_conn.execute(query)

    postgresql_database.exec_transaction(add_root)


@pytest.fixture(scope="function")
def label(api, session):
    r = session.post(api["label"], json=dict(parent_label_id="ROOT", label_id="label_1"))
    assert r.status_code == 201
    return {'id': 'label_1'}
