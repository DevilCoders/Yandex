from __future__ import unicode_literals

import logging
import random
import typing
from infra.yp_service_discovery.api import api_pb2
from infra.yp_service_discovery.python.resolver import resolver

LOGGER = logging.getLogger(__name__)
DEFAULT_SD_TIMEOUT = 3  # seconds
ADDRESS_TYPES = ('fqdn', 'ip6_address', 'ip4_address')

STABLE_CLUSTER_NAMES = ('iva', 'sas', 'vla')
TESTING_CLUSTER_NAMES = ('iva', 'sas')

STABLE_CI_ENDPOINT_SET_ID = 'ci-api-stable.api'
TESTING_CI_ENDPOINT_SET_ID = 'ci-api-testing.api'

MAIN_CLUSTER_NAMES = STABLE_CLUSTER_NAMES
ADDITIONAL_CLUSTER_NAMES = ('man', 'myt')
ALL_CLUSTER_NAMES = MAIN_CLUSTER_NAMES + ADDITIONAL_CLUSTER_NAMES

STABLE_STORAGE_CLUSTER_NAMES = ('iva', 'sas', 'vla')
STABLE_STORAGE_ENDPOINT_SET_ID = 'ci-storage-api-stable.storage-api'


def _make_endpoint_url(endpoint):
    # type: (api_pb2.TEndpoint) -> str
    for address_type in ADDRESS_TYPES:
        address = getattr(endpoint, address_type, None)
        if address:
            break
    else:
        raise Exception('Cannot get address of endpoint: {}'.format(endpoint))

    return '{}:{}'.format(address, endpoint.port)


def _resolve_cluster_endpoint_urls(endpoint_resolver, cluster_name, endpoint_set_id):
    # type: (resolver.Resolver, str, str) -> typing.List[str]
    request = api_pb2.TReqResolveEndpoints()
    request.cluster_name = cluster_name
    request.endpoint_set_id = endpoint_set_id
    result = endpoint_resolver.resolve_endpoints(request)
    cluster_endpoints = [_make_endpoint_url(ep) for ep in result.endpoint_set.endpoints]
    LOGGER.info('Discovered endpoints in %s: %s', cluster_name, cluster_endpoints)
    return cluster_endpoints


def resolve_endpoints(
    endpoint_resolver,
    cluster_names=MAIN_CLUSTER_NAMES,
    endpoint_set_id=STABLE_CI_ENDPOINT_SET_ID,
):
    discovered_endpoints = []
    for cluster_name in cluster_names:
        cluster_endpoints = _resolve_cluster_endpoint_urls(endpoint_resolver, cluster_name, endpoint_set_id)
        discovered_endpoints.extend(cluster_endpoints)

    return discovered_endpoints


def get_all_endpoints(
    client_name,  # type: str
    cluster_names=MAIN_CLUSTER_NAMES,  # type: typing.List[str]
    endpoint_set_id=STABLE_CI_ENDPOINT_SET_ID,  # type: str
    timeout=DEFAULT_SD_TIMEOUT,  # type: int
):
    endpoint_resolver = resolver.Resolver(client_name=client_name, timeout=timeout)
    all_endpoints = resolve_endpoints(endpoint_resolver, cluster_names, endpoint_set_id)
    random.shuffle(all_endpoints)
    LOGGER.info('Available endpoints: %s', all_endpoints)
    return all_endpoints


def select_random_endpoint_in_all_clusters(
    client_name,  # type: str
    cluster_names=MAIN_CLUSTER_NAMES,  # type: typing.List[str]
    endpoint_set_id=STABLE_CI_ENDPOINT_SET_ID,  # type: str
    timeout=DEFAULT_SD_TIMEOUT,  # type: int
):
    """
    Select select random node among all nodes of all clusters.

    Basic method. Should be used by default and especially for biased endpoint distribution.
    :param client_name: String, which uniquely identified api client
    :param cluster_names: Try to resolve only in these clusters
    :param endpoint_set_id: Endpoint set id to use in Resolver
    :param timeout: Resolver timeout
    :return: Randomly chosen endpoint url as string.
    """
    endpoint_resolver = resolver.Resolver(client_name=client_name, timeout=timeout)
    all_endpoints = resolve_endpoints(endpoint_resolver, cluster_names, endpoint_set_id)
    selected_endpoint = random.choice(all_endpoints)
    LOGGER.info('Selected endpoint: %s', selected_endpoint)
    return selected_endpoint


def select_random_endpoint_in_random_cluster(
    client_name,  # type: str
    cluster_names=MAIN_CLUSTER_NAMES,  # type: typing.List[str]
    endpoint_set_id=STABLE_CI_ENDPOINT_SET_ID,  # type: str
    timeout=DEFAULT_SD_TIMEOUT  # type: int
):
    """
    Select random cluster then select random node in it.

    Useful only if endpoint distribution over all clusters is uniform.
    :param client_name: String, which uniquely identified api client
    :param cluster_names: Try to resolve only in these clusters
    :param endpoint_set_id: Endpoint set id to use in Resolver
    :param timeout: Resolver timeout
    :return: Endpoint url as string.
    """
    endpoint_resolver = resolver.Resolver(client_name=client_name, timeout=timeout)
    for random_cluster in random.sample(cluster_names, len(cluster_names)):
        LOGGER.info('Will try to resolve endpoints for %s cluster', random_cluster)
        endpoint_urls = _resolve_cluster_endpoint_urls(endpoint_resolver, random_cluster, endpoint_set_id)
        if endpoint_urls:
            selected_endpoint = random.choice(endpoint_urls)
            LOGGER.info('Selected endpoint: %s', selected_endpoint)
            return selected_endpoint
