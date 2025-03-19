# -*- coding: utf-8 -*-

"""
DBaaS Internal API alerts management
"""

from functools import partial
from itertools import islice
from typing import Iterable, List, Optional

from flask import g
from flask.views import MethodView

from . import API, marshal, parse_kwargs
from .schemas.alerts import (
    ListAlertGroupsRequestSchemaV1,
    ListClusterAlertGroupsRequestSchemaV1,
    GetAlertsGroupRequestSchemaV1,
)
from ..core.auth import check_auth
from ..core.exceptions import (
    AlertGroupNotExistsError,
    DbaasNotImplementedError,
    UnsupportedHandlerError,
    AlertGroupResolveError,
)
from ..utils import metadb, pagination
from ..utils.cluster.get import get_all_clusters_info_in_folder, get_cluster_info_assert_exists
from ..utils.identity import get_folder_by_cluster_id
from ..utils.register import DbaasOperation, Resource, get_request_handler
from ..utils.types import AlertGroup, ClusterInfo, ClusterVisibility, Alert, AlertTemplate


class AlertGroupProxy:
    """
    Proxy alert group attributes
    """

    __slots__ = ('alert_group', 'folder_id', 'cluster_id')

    def __init__(self, alert_group: AlertGroup, folder_id: str, cluster_id: str) -> None:
        self.alert_group = alert_group
        self.folder_id = folder_id
        self.cluster_id = cluster_id

    @property
    def id(self):  # pylint: disable=invalid-name
        """
        Return alert id
        """
        return self.alert_group.alert_group_id

    def __getattr__(self, name):
        return getattr(self.alert_group, name)

    @staticmethod
    def sort_key(agpr):
        """
        Sort key for alert group
        """
        return agpr.alert_group_id


def get_alert_groups_page(
    alert_groups: Iterable[AlertGroupProxy], limit: int, page_token_id: Optional[str]
) -> List[AlertGroupProxy]:
    """
    Return limit elements after page_token_id
    """
    ag_iter = iter(alert_groups)
    if page_token_id is not None:
        for ag in ag_iter:
            if ag.id == page_token_id:
                break
    return list(islice(ag_iter, limit))


def get_alerts_page(alerts: Iterable[Alert], limit: int, page_token_id: Optional[str]) -> List[Alert]:
    """
    Return limit elements after page_token_id
    """
    ag_iter = iter(alerts)
    if page_token_id is not None:
        for ag in ag_iter:
            if ag.template_id == page_token_id:
                break

    return list(islice(ag_iter, limit))


alert_groups_paging = partial(
    pagination.supports_pagination,
    items_field='alertGroups',
    columns=[pagination.AttributeColumn(field='id', field_type=str)],
)

alerts_paging = partial(
    pagination.supports_pagination,
    items_field='alerts',
    columns=[pagination.AttributeColumn(field='template_id', field_type=str)],
)


def get_cluster_alert_groups(info: ClusterInfo) -> list[AlertGroupProxy]:
    """
    Get cluster alerts and wrap then in AlertProxies
    """
    try:
        request_handler = get_request_handler(info.type, Resource.ALERT_GROUP, DbaasOperation.LIST)
    except UnsupportedHandlerError:
        raise DbaasNotImplementedError('Alerts listing for {0} not implemented'.format(info.type))
    alert_groups = request_handler(info)
    folder_id = g.folder['folder_ext_id']
    return [AlertGroupProxy(ag, folder_id, info.cid) for ag in alert_groups]


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>/alert-groups')
class ClusterListAlertGroupsV1(MethodView):
    """
    List cluster alert groups
    """

    @parse_kwargs.with_schema(ListClusterAlertGroupsRequestSchemaV1)
    @marshal.with_resource(Resource.ALERT_GROUP, DbaasOperation.LIST)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.ALERT_GROUP,
        operation=DbaasOperation.LIST,
    )
    @alert_groups_paging()
    def get(
        self, cluster_type: str, cluster_id: str, limit: int, page_token_id: str = None, **_
    ) -> List[AlertGroupProxy]:
        """
        Get cluster alerts
        """
        with metadb.commit_on_success():
            info = get_cluster_info_assert_exists(cluster_id, cluster_type)
            alert_groups = get_cluster_alert_groups(info)
            alert_groups.sort(key=AlertGroupProxy.sort_key)
            return get_alert_groups_page(alert_groups=alert_groups, page_token_id=page_token_id, limit=limit)


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>/alert-groups/<string:alert_group_id>')
class ClusterAlertGroupV1(MethodView):
    """
    Cluster alert groups
    """

    @parse_kwargs.with_schema(GetAlertsGroupRequestSchemaV1)
    @marshal.with_resource(Resource.ALERT_GROUP, DbaasOperation.INFO)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.ALERT_GROUP,
        operation=DbaasOperation.INFO,
    )
    def get(self, cluster_type: str, cluster_id: str, alert_group_id: str, **_) -> AlertGroup:
        """
        Get cluster alert group
        """
        with metadb.commit_on_success():
            get_cluster_info_assert_exists(cluster_id, cluster_type)

            ag = metadb.get_alert_group(ag_id=alert_group_id, cid=cluster_id)

            return AlertGroup(
                alerts=metadb.get_alerts_by_alert_group(ag["alert_group_id"]),
                **ag,
            )


@API.resource('/mdb/<ctype:cluster_type>/<version>/alert-groups')
class ListAlertGroupsV1(MethodView):
    """
    List folder alerts
    """

    @parse_kwargs.with_schema(ListAlertGroupsRequestSchemaV1)
    @marshal.with_resource(Resource.ALERT_GROUP, DbaasOperation.LIST)
    @check_auth(resource=Resource.ALERT_GROUP, operation=DbaasOperation.LIST)
    @alert_groups_paging()
    def get(
        self,
        cluster_type: str,
        limit: int,
        page_token_id: str = None,
        **_,
    ) -> List[AlertGroupProxy]:
        """
        Get folder alert groups
        """
        alerts = []  # type: List[AlertGroupProxy]
        for info in get_all_clusters_info_in_folder(cluster_type, limit=None):
            alerts += get_cluster_alert_groups(info)
        alerts.sort(key=AlertGroupProxy.sort_key)
        return get_alert_groups_page(alert_groups=alerts, page_token_id=page_token_id, limit=limit)


def get_cluster_id_by_alert_id(ag_id: str) -> str:
    """
    Get cluster_id from alert_id
    """
    return metadb.get_cluster_by_ag(ag_id)['cid']


def get_folder_by_alert_group_id(*_, **kwargs):
    """
    Get folder by alert_group_id
    """
    alert_group_id = kwargs.get('alert_group_id')
    if alert_group_id is None:
        raise AlertGroupResolveError('alert_group_id missing')

    cluster_id = get_cluster_id_by_alert_id(alert_group_id)

    ag_cloud, ag_folder = get_folder_by_cluster_id(
        cluster_id=cluster_id,
        visibility=ClusterVisibility.visible_or_deleted,
    )
    return ag_cloud, ag_folder


def get_cluster_alert_by_id(info: ClusterInfo, alert_id: str) -> AlertGroupProxy:
    """
    Get cluster alert by alert_id

    raise NotExists if this alert not found
    """
    for ag in get_cluster_alert_groups(info):
        if ag.alert_group_id == alert_id:
            return ag
    raise AlertGroupNotExistsError(alert_id)


@API.resource('/mdb/<ctype:cluster_type>/<version>/alert-groups/<string:alert_group_id>')
class GetAlertGroupV1(MethodView):
    """
    Get alert info
    """

    @parse_kwargs.with_resource(Resource.ALERT_GROUP, DbaasOperation.INFO)
    @marshal.with_resource(Resource.ALERT_GROUP, DbaasOperation.INFO)
    @check_auth(
        folder_resolver=get_folder_by_alert_group_id,
        resource=Resource.ALERT_GROUP,
        operation=DbaasOperation.INFO,
    )
    def get(self, cluster_type: str, alert_group: str, **_) -> AlertGroupProxy:
        """
        Get alert info by its id
        """
        # don't catch MalformedGlobalAlertId,
        # cause we check_auth by cid extracted from alert_id
        cluster_id = get_cluster_id_by_alert_id(alert_group)
        info = get_cluster_info_assert_exists(cluster_id, cluster_type)
        return get_cluster_alert_by_id(info, alert_group)


@API.resource('/mdb/<ctype:cluster_type>/<version>/alerts-template')
class GetAlertTemplateV1(MethodView):
    """
    Get alert template
    """

    @marshal.with_resource(Resource.ALERT_GROUP, DbaasOperation.ALERTS_TEMPLATE)
    @alerts_paging()
    def get(self, cluster_type: str, **_) -> list[AlertTemplate]:
        """
        Get alert template by cluster type
        """

        return [
            AlertTemplate(**alert)
            for alert in sorted(metadb.get_alert_template(ctype=cluster_type), key=lambda x: x['template_id'])
        ]
