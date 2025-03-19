from django.db import connections
from django.conf import settings
from django.utils.safestring import mark_safe
from django.template import loader
from django.utils.translation import ugettext_lazy as _

from admin_tools.dashboard import Dashboard, modules
from cloud.mdb.ui.internal.common import render_table

if settings.ENABLED_APPS.katan:
    from cloud.mdb.ui.internal.katan.models import Schedule
from cloud.mdb.ui.internal.meta.models import WorkerQueue
from cloud.mdb.ui.internal.omnisearch import search


class CustomIndexDashboard(Dashboard):
    """
    Custom index dashboard for mdbui.
    """

    def init_with_context(self, context) -> None:
        self.children.append(
            modules.LinkList(
                _('Installations'),
                layout='inline',
                draggable=False,
                deletable=False,
                collapsible=False,
                children=[
                    ('Porto Test', 'https://ui.db.yandex-team.ru'),
                    ('Porto Prod', 'https://ui-prod.db.yandex-team.ru'),
                    ('Compute Preprod', 'https://ui-preprod.db.yandex-team.ru'),
                    ('Compute Prod', 'https://ui-compute-prod.db.yandex-team.ru'),
                ],
            )
        )

        self.children.append(FailedTasksModule())
        self.children.append(SearchDashboardModule())

        if settings.ENABLED_APPS.katan:
            if Schedule.objects.filter(state='broken').exists():
                self.children.append(BrokenSchedulesModule())
        self.children.append(PGStatisticsModule())


class SearchDashboardModule(modules.DashboardModule):
    title = 'Search'

    draggable = False
    deletable = False
    collapsible = False

    def init_with_context(self, context):  # noqa
        request = context.request
        q = request.GET.get('q', '')
        result = [
            """
                <form method="get" style="text-align: center; margin: 15px" >
                    <label for="searchbar" style="vertical-align: middle;margin-right: 5px;"><img src="/static/admin/img/search.svg" alt="Search"></label>
                    <input type="text" size="40" name="q" value="%s" autofocus="" placeholder="cid, subcid, fqdn, task_id, shipment, job result">
                    <input type="submit" value="Search" style="padding: 5px 10px">
                </form>
                """
            % q
        ]
        if q:
            search_results = search(q)
            if search_results:
                result.append("<div class='dashboard-module-content'><h3>Found:</h3><ul>")
                for link in search_results:
                    result.append('<li>%s</li>' % link)
                result.append("</ul></div>")
            else:
                result.append('<h3>Not found.</h3>')
        self.children.append(mark_safe('\n'.join(result)))


class FailedTasksModule(modules.DashboardModule):
    title = 'Failed Tasks'

    draggable = False
    deletable = False
    collapsible = False

    def init_with_context(self, _) -> None:
        tasks = WorkerQueue.objects.raw(
            """
            SELECT task_id FROM
            (
                SELECT cid, task_id, status as cluster_status, finish_rev
                    FROM dbaas.worker_queue JOIN dbaas.clusters using (cid)
                    WHERE result=false AND status IN ('CREATE-ERROR', 'MODIFY-ERROR', 'STOP-ERROR', 'START-ERROR', 'DELETE-ERROR', 'PURGE-ERROR', 'METADATA-DELETE-ERROR')
            ) tasks
            JOIN dbaas.clusters_revs cr ON (tasks.cid = cr.cid AND tasks.finish_rev = cr.rev)
                WHERE dbaas.error_cluster_status(status)
        """
        )
        self.children.append(
            mark_safe(
                render_table(
                    ['cid / status', 'task id', 'task type', 'create_ts'],
                    [
                        [
                            '<a href="/meta/cluster/{cid}">{cid}</a> &nbsp;<small>{status}</small>'.format(
                                cid=t.cid, status=t.cid.get_status_display()
                            ),
                            '<a href="/meta/workerqueue/{task_id}">{task_id}</a>'.format(task_id=t.task_id),
                            t.task_type,
                            t.create_ts.strftime('%d.%m.%y %H:%M'),
                        ]
                        for t in tasks
                    ],
                    True,
                )
            )
        )


class BrokenSchedulesModule(modules.DashboardModule):
    title = 'Broken Schedules'

    draggable = False
    deletable = False
    collapsible = False

    def init_with_context(self, _) -> None:
        schedules = Schedule.objects.filter(state='broken')
        self.children.append(
            mark_safe(
                render_table(
                    ['schedule', 'namespace', 'name', 'rollout'],
                    [
                        [
                            '<a href="/katan/schedule/{schedule_id}">{schedule_id}</a>'.format(
                                schedule_id=s.schedule_id
                            ),
                            s.namespace,
                            s.name,
                            '<a href="/katan/rollout/{rollout_id}">{rollout_id}</a>'.format(
                                rollout_id=s.last_rollout_id
                            ),
                        ]
                        for s in schedules
                    ],
                    True,
                )
            )
        )


class PGStatisticsModule(modules.DashboardModule):
    title = 'Postgresql Statistic'

    draggable = False
    deletable = False
    collapsible = False

    def init_with_context(self, _) -> None:
        with connections['meta_slave'].cursor() as cursor:
            cursor.execute(
                """
                   select component, major_version, minor_version, count(*) from versions join clusters using (cid)
                   where type = 'postgresql_cluster' group by major_version, minor_version, component
                   order by component, major_version, minor_version
               """
            )
            versions = cursor.fetchall()

            cursor.execute(
                """
                 select env, mt.status, count(*) from dbaas.versions v join dbaas.clusters c using (cid)
                 join dbaas.default_versions dv using (type, env, major_version, edition) left join
                 (select * from dbaas.maintenance_tasks where config_id = 'postgresql_update_minor_version') mt on
                 mt.cid = c.cid where c.type = 'postgresql_cluster' and dv.component = 'postgres' and v.component = 'postgres'
                 and dbaas.visible_cluster_status(c.status) and (v.minor_version != dv.minor_version or
                 mt.status = 'COMPLETED') group by env, mt.status order by 1, 2
               """
            )
            mt = cursor.fetchall()

        self.children.append(
            loader.get_template('admin/pg.html').render(
                {
                    'versions': versions,
                    'maint': mt,
                }
            )
        )
