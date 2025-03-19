#!/usr/bin/env python3
"""
Utility functions applicable to both py-intapi and go-intapi.
"""
from tests.helpers import go_internal_api, internal_api
from tests.helpers.compute_driver import get_compute_api


def load_cluster_into_context(context):
    """
    Loads cluster info from int api (go or py) and saves into context.
    """
    if context.cluster_type in ('kafka', 'sqlserver', 'elasticsearch'):
        go_internal_api.load_cluster_into_context(context, context.cluster_type)
    else:
        internal_api.load_cluster_into_context(context)


# pylint: disable=invalid-name
def get_compute_instances_of_cluster(context, view='BASIC'):
    """
    Returns the list of compute instances of the cluster.
    """
    load_cluster_into_context(context)
    compute_api = get_compute_api(context)
    return [compute_api.get_instance(host['name'], view=view) for host in context.hosts]
