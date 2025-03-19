"""
Solomon API interaction module
"""

from typing import Iterable, Optional

from ..common import Change
from dbaas_common import retry, tracing
from ...exceptions import ExposedException
from ..http import HTTPClient, HTTPErrorHandler
from .models import Alert, req_from_alert, req_update_template_version_from_alert, req_stub_complete_request, Template

from dataclasses import asdict


class SolomonApiError(ExposedException):
    """
    Base solomon error
    """


def service_prov_header(config) -> tuple[str, str]:
    return 'X-Service-Provider', config.solomon.service_provider_id


class SolomonApiV2(HTTPClient):
    """
    Solomon provider
    """

    def __init__(self, config, task, queue, solomon_project: Optional[str] = None):
        super().__init__(config, task, queue)
        self.ident_template = self.config.solomon.ident_template
        if self._disabled:
            return
        headers = {
            'Authorization': 'OAuth {token}'.format(token=self.config.solomon.token),
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.solomon_project = solomon_project or self.config.solomon.project
        self._init_session(
            self.config.solomon.url,
            'api/v2/projects/',
            default_headers=headers,
            error_handler=HTTPErrorHandler(SolomonApiError),
            verify=self.config.solomon.ca_path,
        )

    @retry.on_exception(SolomonApiError, factor=10, max_wait=60, max_tries=3)
    def _delete_alert(self, alert: Alert):
        if len(alert.ext_id) == 0:
            # alert was not created
            return
        self._make_request(
            f'{alert.project_id}/alerts/{alert.ext_id}',
            headers=dict([service_prov_header(self.config)]),
            method='DELETE',
            expect=[200, 204, 404],
        )

    @tracing.trace('Solomon AlertsFullModel')
    def alerts_list_by_labels(self, cid: str, prj_id: str) -> list[Alert]:
        ret = []

        resp = self._make_request(
            f'{prj_id}/alertsFullModel',
            headers=dict([service_prov_header(self.config)]),
            method='GET',
            params={'labelsSelector': 'cluster=' + cid},
            expect=[200, 204, 404],
        )['items']

        for el in resp:
            ret.append(
                Alert(
                    ext_id=el['id'],
                    name=el['name'],
                    project_id=el['projectId'],
                    alert_group_id='',
                    state=el['state'],
                    notification_channels=el['notificationChannels'],
                    description=el['description'],
                    template=Template(
                        id='',
                        version='',
                    ),
                )
            )

        return ret

    @tracing.trace('Solomon Alerts Deleted')
    def delete_alerts_by_id(self, alerts: Iterable[Alert]):
        for alert in alerts:
            context_key = f'solomon-alert.delete.{alert.ext_id}'
            context = self.context_get(context_key)
            if context:
                self.add_change(Change(context_key, 'deleted'))
                continue
            self._delete_alert(alert)
            self.add_change(
                Change(
                    context_key,
                    'deleted',
                    {context_key: alert.ext_id},
                    rollback=Change.noop_rollback,
                )
            )

    @retry.on_exception(SolomonApiError, factor=10, max_wait=60, max_tries=3)
    def _modify_alert(self, alert: Alert, cid: str):
        """
        Ensure current template version and modify alert to the supplied version.
        """
        response = self._make_request(
            f'{alert.project_id}/alerts/{alert.ext_id}', headers=dict([service_prov_header(self.config)]), method='GET'
        )
        curr_version = response['type']['fromTemplate']['templateVersionTag']
        if curr_version != alert.template.version:
            self._make_request(
                f'{alert.project_id}/alerts/{alert.ext_id}/update-template-version',
                headers=dict([service_prov_header(self.config)]),
                data=asdict(req_update_template_version_from_alert(alert, cid)),
                method='POST',
            )
        self._make_request(
            f'{alert.project_id}/alerts/{alert.ext_id}',
            headers=dict([service_prov_header(self.config)]),
            data=asdict(req_from_alert(alert, cid)),
            method='PUT',
        )

    @tracing.trace('Solomon Alerts Modified')
    def modify_alerts(self, alerts: Iterable[Alert], cid: str):
        for alert in alerts:
            context_key = f'solomon-alert.modify.{alert.ext_id}'
            context = self.context_get(context_key)
            if context:
                self.add_change(Change(context_key, 'modified'))
                continue
            self._modify_alert(alert, cid)
            self.add_change(
                Change(
                    context_key,
                    'modified',
                    {context_key: alert.ext_id},
                    rollback=Change.noop_rollback,
                )
            )

    @retry.on_exception(SolomonApiError, factor=10, max_wait=60, max_tries=3)
    def _create_service_alert(self, alert: Alert, cid: str):
        response = self._make_request(
            f'{alert.project_id}/alerts',
            headers=dict([service_prov_header(self.config)]),
            data=asdict(req_from_alert(alert, cid)),
            method='POST',
        )
        alert.ext_id = response['id']

    @tracing.trace('Solomon Alerts Created')
    def create_service_alerts(self, alerts: Iterable[Alert], cid: str):
        for alert in alerts:
            context_key = f'solomon-alert.create.{alert.template.id}{alert.alert_group_id}'
            context = self.context_get(context_key)
            if context:
                self.add_change(Change(context_key, 'created'))
                continue
            self._create_service_alert(alert, cid)
            self.add_change(
                Change(
                    context_key,
                    'created',
                    {context_key: alert.ext_id},
                    rollback=Change.noop_rollback,
                )
            )

    @property
    def _disabled(self) -> bool:
        return not self.config.solomon.enabled

    def _get_cluster(self, cluster_id):
        """
        Get cluster in solomon
        """
        res = self._make_request(
            f'{self.solomon_project}/clusters/{cluster_id}',
            expect=[200, 404],
        )
        if res.get('code') != 404:
            return res

    def _get_shard(self, shard_id):
        """
        Get shard in solomon
        """
        res = self._make_request(
            f'{self.solomon_project}/shards/{shard_id}',
            expect=[200, 404],
        )
        if res.get('code') != 404:
            return res

    def _delete_cluster(self, cluster_id):
        """
        Delete cluster from solomon
        """
        self._make_request(f'{self.solomon_project}/clusters/{cluster_id}', 'delete', expect=[204])

    def _delete_shard(self, shard_id):
        """
        Delete shard from solomon
        """
        self._make_request(f'{self.solomon_project}/shards/{shard_id}', 'delete', expect=[204])

    def cluster_absent(self, cluster_id, shard_id=None):
        """
        Ensure that cluster and shard absent
        """
        if self._disabled:
            return
        try:
            self._must_delete_cluster(cluster_id, shard_id)
        except Exception:
            self.logger.exception('we tried our best to delete solomon cluster, but failed')

    @retry.on_exception(SolomonApiError, factor=10, max_wait=60, max_tries=3)
    @tracing.trace('Solomon Cluster Absent')
    def _must_delete_cluster(self, cluster_id, shard_id=None):
        """
        Ensure that cluster and shard absent
        """
        tracing.set_tag('cluster.id', cluster_id)
        if shard_id:
            tracing.set_tag('cluster.shard.id', shard_id)

        cluster_id = self.ident_template.format(ident=cluster_id)
        shard_id = self.ident_template.format(ident=shard_id) if shard_id else cluster_id
        autogen_cluster_id = '{project}_{cluster_id}'.format(project=self.config.solomon.project, cluster_id=cluster_id)
        autogen_shard_id = '{project}_{cluster_id}_{service_label}'.format(
            project=self.config.solomon.project, cluster_id=cluster_id, service_label=self.config.solomon.service_label
        )
        if self._get_shard(shard_id):
            self._delete_shard(shard_id)
        if self._get_shard(autogen_shard_id):
            self._delete_shard(autogen_shard_id)

        if self._get_cluster(autogen_cluster_id):
            self._delete_cluster(autogen_cluster_id)
        if self._get_cluster(cluster_id):
            self._delete_cluster(cluster_id)


class SolomonApiV3(HTTPClient):
    """
    Solomon provider
    """

    def __init__(self, config, task, queue, solomon_project: Optional[str] = None):
        super().__init__(config, task, queue)
        self.ident_template = self.config.solomon.ident_template
        headers = {
            'Authorization': 'OAuth {token}'.format(token=self.config.solomon.token),
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.solomon_project = solomon_project or self.config.solomon.project
        self._init_session(
            self.config.solomon.url,
            'api/v3/',
            default_headers=headers,
            error_handler=HTTPErrorHandler(SolomonApiError),
            verify=self.config.solomon.ca_path,
        )

    @tracing.trace('Solomon Complete Stub Request')
    @retry.on_exception(SolomonApiError, factor=10, max_wait=60, max_tries=3)
    def complete_stud_request(self, cid: str, stub_id_request: str):
        _ = self._make_request(
            f'stubs/{stub_id_request}/alerts/complete',
            headers=dict([service_prov_header(self.config)]),
            data=asdict(req_stub_complete_request(cid)),
            method='POST',
        )
