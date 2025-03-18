"""
This file contains context object represent context GOF pattern. It eases testing and dependencies managment
"""
from antiadblock.configs_api.lib.auth.auth import Auth
from antiadblock.configs_api.lib.auth.blackbox import BlackboxClient, Blackbox
from antiadblock.configs_api.lib.auth.webmaster import WebmasterAPI
from antiadblock.configs_api.lib.metrics.metrics_client import MetricsClient
from antiadblock.configs_api.lib.utils import CryproxClient
from antiadblock.configs_api.lib.stat.charts_api import ChartsClient
from antiadblock.configs_api.lib.argus.argus import ArgusClient
from antiadblock.configs_api.lib.sonar.sonar import SonarClient
from antiadblock.configs_api.lib.infra.configs_infra_handler import ConfigsInfraHandler
from antiadblock.libs.infra.lib.infra_client import InfraClient
from antiadblock.libs.utils.utils import get_abc_duty

from startrek_client import Startrek

from tvmauth import TvmClient, TvmApiClientSettings


class Context(object):
    def __init__(self):
        self.auth = None
        self.webmaster = None
        self.metrics = None
        self.cryprox = None
        self.blackbox = None
        self.tvm = None
        self.charts_client = None
        self.argus_client = None
        self.sonar_client = None
        self.startrek_client = None
        self.infra_handler = None
        self.get_abc_duty = get_abc_duty

    def init_webmaster(self, app):
        self.webmaster = WebmasterAPI(self, app.config["WEBMASTER_API_URL"])

    def init_metrics(self, app):
        # TODO: https://st.yandex-team.ru/ANTIADB-1320. Take metrics from solomon instead of elastic
        self.metrics = MetricsClient()

    def init_auth(self, app):
        self.auth = Auth(self)

    def init_cryprox(self, app):
        self.cryprox = CryproxClient()

    def init_startrek(self, app):
        self.startrek_client = Startrek(useragent='Configs API', token=app.config["TOOLS_TOKEN"])

    def init_blackbox(self, app):
        self.blackbox = BlackboxClient(self._get_blackbox_client_type(app), self)

    def init_tvm(self, app):
        tvm_settings = TvmApiClientSettings(
            self_tvm_id=app.config["AAB_ADMIN_TVM_CLIENT_ID"],
            self_secret=app.config["TVM_SECRET"],
            enable_service_ticket_checking=True,
            enable_user_ticket_checking=self._get_blackbox_client_type(app).env,
            dsts=dict(blackbox=int(self._get_blackbox_client_type(app).tvm_client_id), webmaster=app.config["WEBMASTER_TVM_CLIENT_ID"]))

        self.tvm = TvmClient(tvm_settings)

    def init_charts_api(self, app):
        self.charts_client = ChartsClient(oauth_token=app.config["CHARTS_TOKEN"],
                                          is_prod=app.config["IS_PROD"])

    def init_argus(self, app):
        self.argus_client = ArgusClient(oauth_token=app.config["SANDBOX_TOKEN"])

    def init_sonar(self, app):
        self.sonar_client = SonarClient(oauth_token=app.config["YT_TOKEN"])

    def init_infra(self, app):
        infra_client = InfraClient(infra_api_url_prefix=app.config["INFRA_API_URL"],
                                   oauth_token=app.config["INFRA_TOKEN"])
        self.infra_handler = ConfigsInfraHandler(infra_client=infra_client,
                                                 environment_type=app.config["ENVIRONMENT_TYPE"],
                                                 host_domain=app.config["HOST_DOMAIN"],
                                                 namespace_id=app.config["INFRA_NAMESPACE_ID"])

    @staticmethod
    def _get_blackbox_client_type(app):
        if app.config["ENVIRONMENT_TYPE"] == "PRODUCTION":
            return Blackbox.Prod
        elif app.config["ENVIRONMENT_TYPE"] == "TESTING":
            return Blackbox.Mimino
        else:
            return Blackbox.Stub


CONTEXT = Context()


def init_context(app):
    CONTEXT.init_metrics(app)
    CONTEXT.init_auth(app)
    CONTEXT.init_webmaster(app)
    CONTEXT.init_cryprox(app)
    CONTEXT.init_tvm(app)
    CONTEXT.init_blackbox(app)
    CONTEXT.init_charts_api(app)
    CONTEXT.init_argus(app)
    CONTEXT.init_sonar(app)
    CONTEXT.init_startrek(app)
    CONTEXT.init_infra(app)
