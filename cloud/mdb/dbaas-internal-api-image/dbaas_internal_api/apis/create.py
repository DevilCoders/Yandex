# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster create
"""

from flask import g
from flask.views import MethodView
from flask_restful import abort

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..utils import metadb
from ..utils.idempotence import supports_idempotence
from ..utils.register import DbaasOperation, Resource, get_request_handler
from .operations import render_operation_v1
from .schemas.operations import OperationSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters')
class CreateClusterV1(MethodView):
    """Create new database cluster"""

    @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.CREATE)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(resource=Resource.CLUSTER, operation=DbaasOperation.CREATE)
    @supports_idempotence
    def post(self, cluster_type, **db_specific_params):
        """
        Call function matching by cluster_type
        """
        # NOTE: We get lock on cloud here because cluster
        # creation assumes that cloud resources will be
        # changed and we will update cloud.
        # NOTE: g.cloud is also updated
        if not metadb.lock_cloud():
            abort(403)

        handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.CREATE)
        # see custom `@use_kwargs` above for schema validation details.
        operation = handler(**db_specific_params)

        metadb.complete_cluster_change(operation.target_id)
        g.metadb.commit()
        return render_operation_v1(operation)
