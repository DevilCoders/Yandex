# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster topology info
"""

import marshmallow
from flask.views import MethodView
from webargs.flaskparser import use_kwargs

from ...apis import API
from ...apis.config_auth import check_auth
from ...apis.schemas.fields import Str
from ...utils.cluster.get import get_cluster_info
from ...utils.metadb import get_folder_by_cluster, get_instance_groups
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import ClusterService


@API.resource('/mdb/hadoop/1.0/clusters/<string:cid>/topology')
class HadoopTopology(MethodView):
    """
    Minion config (used as ext_pillar)
    """

    @use_kwargs(
        {
            'access_id': marshmallow.fields.UUID(required=True, location='headers', load_from='Access-Id'),
            'access_secret': Str(required=True, location='headers', load_from='Access-Secret'),
        }
    )
    def get(self, cid, access_id, access_secret):
        """
        Check config host auth and return config
        """
        check_auth(access_id, access_secret, ('dataproc-api', 'dataproc-ui-proxy'))
        cluster = get_cluster_info(cid, MY_CLUSTER_TYPE)
        folder = get_folder_by_cluster(cid=cid)
        pillar = get_cluster_pillar(cluster)
        instance_groups = get_instance_groups(cid)
        instance_group_by_subcid = {row['subcid']: row['instance_group_id'] for row in instance_groups}
        for subcluster in pillar.subclusters:
            if subcluster['subcid'] in instance_group_by_subcid:
                subcluster['instance_group_id'] = instance_group_by_subcid[subcluster['subcid']]
                subcluster['decommission_timeout'] = subcluster.get('decommission_timeout')
        return {
            'revision': pillar.topology_revision,
            'folder_id': folder['folder_ext_id'],
            'services': ClusterService.to_strings(pillar.services),
            'subcluster_main': pillar.subcluster_main,
            'subclusters': pillar.subclusters,
            'ui_proxy': pillar.ui_proxy,
            'service_account_id': pillar.service_account_id,
        }
