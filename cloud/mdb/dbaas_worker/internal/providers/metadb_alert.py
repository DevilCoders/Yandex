"""
MetaDB alert manipulation module
"""

from typing import Iterable
from contextlib import contextmanager

from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .solomon_client.models import Alert, AlertState, Template, Param


class MetadbAlert(BaseMetaDBProvider):
    """
    DBaaS MetaDB alert provider
    """

    @contextmanager
    def cursor(self):
        with self.get_master_conn() as conn:
            with conn:
                yield conn.cursor()

    def set_alerts_active(self, alerts: Iterable[Alert]):
        with self.cursor() as cur:
            for alert in alerts:
                execute(
                    cur,
                    'set_alert_active',
                    fetch=False,
                    **{
                        'group_id': alert.alert_group_id,
                        'template_id': alert.template.id,
                        'ext_id': alert.ext_id,
                    },
                )

    def delete_alerts(self, alert_group_id: str, template_ids: Iterable[str]):
        with self.cursor() as cur:
            execute(
                cur,
                'alert_delete_by_id',
                fetch=False,
                **{
                    'alert_group_id': alert_group_id,
                    'template_ids': template_ids,
                },
            )

    def get_cluster_project_id(self, cid: str) -> str:
        with self.cursor() as cur:
            ret = execute(cur, 'get_cluster_project_id', fetch=True, cid=cid)

        prj_id = ret[0]['monitoring_cloud_id'] if len(ret) == 1 else ''
        return prj_id

    def get_alerts_by_cid(self, cid: str) -> list[Alert]:
        with self.cursor() as cur:
            ret = execute(
                cur,
                'get_alerts_by_cid',
                fetch=True,
                **{
                    'cid': cid,
                },
            )
        result = []
        for row in ret:
            result.append(
                Alert(
                    ext_id=row['alert_ext_id'] or '',
                    project_id=row['monitoring_folder_id'],
                    state=AlertState(row['status']),
                    name='{} ({})'.format(row['name'], cid),
                    description=row['description'],
                    template=Template(
                        id=row['template_id'],
                        version=row['template_version'],
                        doubleValueThresholds=[
                            Param(name='warnThreshold', value=str(row['warning_threshold'])),
                            Param(name='alarmThreshold', value=str(row['critical_threshold'])),
                        ],
                        textValueParameters=[
                            Param(name='cluster', value='{cid}'.format(cid=cid)),
                        ],
                    ),
                    notification_channels=row['notification_channels'],
                    alert_group_id=row['alert_group_id'],
                )
            )
        return result
