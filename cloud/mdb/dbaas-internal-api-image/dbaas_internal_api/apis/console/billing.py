# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster create billing estimator
"""
from typing import Callable

from flask.views import MethodView

from .. import API, marshal, parse_kwargs
from ...core.auth import check_auth
from ...core.exceptions import DbaasNotImplementedError, UnsupportedHandlerError
from ...utils.metadb import commit_on_success
from ...utils.register import DbaasOperation, Resource, get_request_handler
from ..schemas.console import EstimateCreateResponseSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/console/clusters:estimate')
class EstimateCreateClusterV1(MethodView):
    """
    Estimate cost of database cluster
    """

    @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
    @marshal.with_schema(EstimateCreateResponseSchemaV1)
    @check_auth(resource=Resource.CLUSTER, operation=DbaasOperation.BILLING_CREATE)
    def post(self, cluster_type: str, **cluster_type_specific_args) -> dict:
        """
        Estimate billing cost of cluster
        """
        with commit_on_success():
            try:
                handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
                return _gen_metrics(handler, **cluster_type_specific_args)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Billing estimate for {0} is not implemented'.format(cluster_type))


@API.resource('/mdb/<ctype:cluster_type>/<version>/console/hosts:estimate')
class EstimateCreateHostsV1(MethodView):
    """
    Estimate cost of one or more hosts already belonging to an existing cluster
    """

    @parse_kwargs.with_resource(Resource.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
    @marshal.with_schema(EstimateCreateResponseSchemaV1)
    @check_auth(resource=Resource.HOST, operation=DbaasOperation.BILLING_CREATE_HOSTS)
    def post(self, cluster_type: str, **cluster_type_specific_args) -> dict:
        """
        Estimate billing cost of separate hosts (for bulk operations, e.g. enableSharding)
        """
        with commit_on_success():
            try:
                handler = get_request_handler(cluster_type, Resource.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
                return _gen_metrics(handler, **cluster_type_specific_args)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Hosts billing estimate for {0} is not implemented'.format(cluster_type))


def _gen_metrics(handler: Callable, **cluster_type_specific_args) -> dict:
    metrics = list(handler(**cluster_type_specific_args))
    return {'metrics': metrics}
