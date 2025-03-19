from yc_common import config, logging
from yc_common.metrics import Metric, MetricTypes, monitor
from yc_common.models import Model, StringType
from yc_common.ya_clients import exception
from yc_common.ya_clients.common import IntranetClient


log = logging.get_logger(__name__)

requests = Metric(
    MetricTypes.COUNTER, "cauth_client_requests",
    ["type", "path"], "CAuth client request counter.")


class CAuthEndpointConfig(Model):
    cert = StringType(required=True)
    key = StringType(required=True)
    base_url = StringType(default="https://cauth-api.yandex-team.ru")


class CAuthClient(IntranetClient):
    _client_name = "cauth"

    def __init__(self, certfile, keyfile, base_url, timeout):
        for cert_file in (certfile, keyfile):
            try:
                cert_fd = open(cert_file, "r")
            except OSError as oserr:
                raise exception.ConfigurationError(self._client_name, "Unable to open: {}".format(oserr))
            else:
                cert_fd.close()
        super().__init__(base_url=base_url, auth_type="cert", certfile=certfile, keyfile=keyfile, timeout=timeout)

    @monitor(requests, labels={"path": "add_server"})
    def add_server(self, fqdn, responsible, groups=None):
        log.debug("Registering server %s in CAuth", fqdn)
        params = {"srv": fqdn, "resp": ",".join(responsible)}
        if groups:
            params["grp"] = groups
        return self.post("add_server", data=params)

    @monitor(requests, labels={"path": "remove_server"})
    def remove_server(self, fqdn):
        log.debug("Deregistering server %s in CAuth", fqdn)
        return self.post("remove_server", data={"srv": fqdn})


def get_cauth_client(timeout=5) -> CAuthClient:
    cauth_conf = config.get_value("endpoints.cauth", model=CAuthEndpointConfig)
    return CAuthClient(certfile=cauth_conf.cert,
                       keyfile=cauth_conf.key,
                       base_url=cauth_conf.base_url,
                       timeout=timeout)
