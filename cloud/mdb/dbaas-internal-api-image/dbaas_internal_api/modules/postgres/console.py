# -*- coding: utf-8 -*-
"""
PostgreSQL handlers for console API.
"""
from flask.views import MethodView

from . import utils
from ...apis import API
from ...core.auth import check_auth
from ...utils.cluster.get import get_cluster_info
from ...utils.identity import get_folder_by_cluster_id
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.version import fill_updatable_to_metadb
from ...utils.metadb import get_default_versions
from .constants import MY_CLUSTER_TYPE
from .pillar import PostgresqlClusterPillar
from .validation import UPGRADE_PATHS, EXTENSIONS_LIST


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
def get_clusters_config_handler(res: dict) -> None:
    """
    Performs PostgreSQL specific logic for 'get_clusters_config'
    """
    #
    # THIS METHOD IS DEPRECATED
    # Add new features to go-api
    # In this METHOD we don't know current environment, using qa by default
    metadb_default_versions = get_default_versions(component='postgres', env='qa', type=MY_CLUSTER_TYPE)
    fill_updatable_to_metadb(res['available_versions'], UPGRADE_PATHS, metadb_default_versions)


@API.resource('/mdb/postgresql/1.0/console/clusters/<string:cluster_id>/users:for_grants')
class GrantsForNewUser(MethodView):
    @check_auth(explicit_action='mdb.all.read', folder_resolver=get_folder_by_cluster_id)
    def get(self, cluster_id):
        """
        Returns list of postgresql users with mdb-users
        """
        cluster = get_cluster_info(cluster_id, MY_CLUSTER_TYPE)
        cluster_pillar = PostgresqlClusterPillar(cluster.value)

        return [role for role in utils.get_all_available_roles(cluster_pillar)]


@API.resource('/mdb/postgresql/1.0/console/clusters/<string:cluster_id>/users/<string:username>:for_grants')
class GrantsForUser(MethodView):
    @check_auth(explicit_action='mdb.all.read', folder_resolver=get_folder_by_cluster_id)
    def get(self, cluster_id, username):
        """
        Returns list of postgresql users with mdb-users
        """
        cluster = get_cluster_info(cluster_id, MY_CLUSTER_TYPE)
        cluster_pillar = PostgresqlClusterPillar(cluster.value)
        user_grants = set(cluster_pillar.pgusers.public_user(username).grants)
        all_roles = utils.get_all_available_roles(cluster_pillar)

        return [g for g in utils.get_assignable_grants(username, all_roles) if g not in user_grants]


@API.resource('/mdb/postgresql/1.0/console/clusters/<string:cluster_id>:available_extensions')
class AvailableExtensionsList(MethodView):
    @check_auth(explicit_action='mdb.all.read', folder_resolver=get_folder_by_cluster_id)
    def get(self, cluster_id):
        """
        Returns list of extensions available to install in cluster
        """
        pg_version = utils.get_cluster_major_version(cluster_id)
        return EXTENSIONS_LIST[pg_version]
