import time

from ..common import BaseProvider
from ..iam_jwt import IamJwt

from dbaas_common import tracing
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python import vpc


class MdbVpcDisabledError(RuntimeError):
    """
    mdb-vpc provider not initialized. Enable it in config'
    """


class VPC(BaseProvider):
    _vpc = None

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if config.mdb_vpc.enabled:
            iam_jwt = IamJwt(
                config,
                task,
                queue,
            )
            self._vpc = vpc.API(
                vpc.Config(
                    transport=grpcutil.Config(
                        url=config.mdb_vpc.url,
                        cert_file=config.mdb_vpc.cert_file,
                        server_name=config.mdb_vpc.server_name,
                        insecure=config.mdb_vpc.insecure,
                    ),
                ),
                self.logger,
                iam_jwt.get_token,
            )

    @tracing.trace("Await network is active")
    def await_network(self, network_id: str, timeout=300.0, delay=2.0) -> vpc.Network:
        if self._vpc is None:
            raise MdbVpcDisabledError

        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                net = self._vpc.get_network(network_id)
                if net.status == vpc.NetworkStatus.ACTIVE:
                    return net
                elif net.status == vpc.NetworkStatus.CREATING:
                    self.logger.debug("Network %s is not active, sleep for %d sec", net.network_id, delay)
                    time.sleep(delay)
                else:
                    raise RuntimeError(f"There is network '{network_id}' in unsupported status: {net.status}")
            except vpc.VPCNotFound:
                self.logger.debug("Network %s not found", network_id)
                time.sleep(delay)
        else:
            raise RuntimeError(f"Timeout exceeded, network {network_id} is still not active")
