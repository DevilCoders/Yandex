from yc_common import config
from yc_common.clients.network.client import NativeNetworkClient
from yc_common.clients.network.config import ClusterNetworkConfig, NetworkEndpointConfig
from yc_common.exceptions import LogicalError


class NetworkClientPool:
    def __init__(self, request_id: str = None, operation_id: str = None):
        self.__client_cache = {}
        self._request_id = request_id
        self._operation_id = operation_id

    def __getitem__(self, zone_id: str) -> NativeNetworkClient:
        if zone_id not in self.__client_cache:
            self.__client_cache[zone_id] = get_network_client(zone_id, self._request_id, self._operation_id)
        return self.__client_cache[zone_id]


def get_network_client_pool(request_id: str = None, operation_id: str = None) -> NetworkClientPool:
    return NetworkClientPool(request_id, operation_id)


def get_network_client(zone_id, request_id: str = None, operation_id: str = None) -> NativeNetworkClient:
    network_config = config.get_value("endpoints.network", model=NetworkEndpointConfig)
    cluster = _get_default_cluster(network_config, zone_id)
    host = cluster["host"]
    port = cluster["port"]

    return NativeNetworkClient(
        host=host,
        port=port,
        protocol=network_config.protocol,
        schema_version=network_config.schema_version,
        request_id=request_id,
        operation_id=operation_id,
    )


def _get_default_cluster(network_config: NetworkEndpointConfig, zone_id) -> ClusterNetworkConfig:
    """Get default cluster record for az config."""

    for zone in network_config.zones:
        if zone["name"] == zone_id:
            return zone.oct_clusters[0]

    raise LogicalError("Unable to get default Contrail cluster for zone {!r}.", zone_id)
