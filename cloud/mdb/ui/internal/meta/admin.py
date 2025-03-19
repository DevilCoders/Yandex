import json
from enum import Enum

from django.conf import settings
from django.contrib import admin
from django.template import loader
from django.utils.safestring import mark_safe
from django_admin_inline_paginator.admin import PaginationFormSetBase
from django_object_actions import DjangoObjectActions

from cloud.mdb.ui.internal.common import (
    ROAdmin,
    ROStackedInline,
    ROTablularInline,
    get_cluster_links,
    get_host_links,
    make_jaeger_link,
    make_spoiler,
    render_change,
    render_pillar,
    render_pillar_revs,
    render_table,
    size_pretty,
    with_copy,
)
from cloud.mdb.ui.internal.deploy.admin import get_job_id_link
from cloud.mdb.ui.internal.deploy.models import Command, Job, ShipmentCommand

if settings.ENABLED_APPS.cms:
    from cloud.mdb.ui.internal.cms.admin import walle_link
from cloud.mdb.ui.internal.meta import models
from cloud.mdb.ui.internal.meta import logic
from cloud.mdb.ui.internal.omnisearch import search_by_fqdn, get_cms_last_decision


__FLAVORS_CACHE = {}


def _get_flavors() -> dict[str, models.Flavor]:
    if not __FLAVORS_CACHE:
        for f in models.Flavor.objects.all():
            __FLAVORS_CACHE[f.id] = f
    return __FLAVORS_CACHE


__GEO_CACHE = {}


def _get_geo() -> dict[str, models.Geo]:
    if not __GEO_CACHE:
        for g in models.Geo.objects.all():
            __GEO_CACHE[g.geo_id] = g
    return __GEO_CACHE


MAX_SHOW_PILLAR_REVS = 40


def link(model_name, id, value):
    return mark_safe('<a href=/meta/%s/%s/>%s</a>' % (model_name, id, value))


class ClustersInline(ROTablularInline):
    fields = ('cid_link', 'name', 'type', 'env', 'created_at', 'folder', 'status')
    model = models.Cluster

    def cid_link(self, obj):
        return obj.link

    cid_link.short_description = 'Cluster'

    def get_queryset(self, request):
        qs = super().get_queryset(request)
        return qs.exclude(status__in=['PURGED', 'DELETED', 'METADATA-DELETED'])


class WorkerQueueInline(ROTablularInline):
    model = models.WorkerQueue
    fields = ('task_link', 'create_ts', 'task_type', 'result')
    ordering = ('-create_ts',)
    template = 'admin/tabular_paginated.html'
    pagination_key = 'page'
    per_page = 10

    def task_link(self, obj):
        return obj.link

    task_link.short_description = 'Task ID'

    def get_formset(self, request, obj=None, **kwargs):
        formset_class = super().get_formset(request, obj, **kwargs)

        class PaginationFormSet(PaginationFormSetBase, formset_class):
            pagination_key = self.pagination_key

        PaginationFormSet.request = request
        PaginationFormSet.per_page = self.per_page
        return PaginationFormSet


class VersionFilter(admin.SimpleListFilter):
    title = 'Version'
    parameter_name = 'version'

    def lookups(self, request, model_admin):
        result = []
        for row in models.Version.objects.distinct('component', 'major_version', 'minor_version').values(
            'component', 'major_version', 'minor_version'
        ):
            values = row['component'], row['major_version'], row['minor_version']
            result.append(('_'.join(values), ' '.join(values)))
        return result

    def queryset(self, request, queryset):
        value = self.value()
        if value is None:
            return queryset
        component, major_version, minor_version = value.split('_')
        cids = models.Version.objects.filter(
            component=component, major_version=major_version, minor_version=minor_version
        ).values('cid')
        queryset = queryset.filter(cid__in=[x['cid'] for x in cids])
        return queryset


class FoldersInline(ROStackedInline):
    inlines = (ClustersInline,)
    fields = ('folder_id', 'folder_ext_id', 'cloud')
    model = models.Folder


class MaintenanceTaskInline(ROTablularInline):
    model = models.MaintenanceTask
    fields = ('cid', 'config_id', 'task_id', 'create_ts', 'plan_ts', 'status', 'info')


class CloudsAdmin(ROAdmin):
    inlines = (FoldersInline,)
    list_display = ('cloud_id', 'cloud_ext_id', 'clusters_link')
    search_fields = ('cloud_ext_id',)
    fields = ('cloud_id_copy', 'cloud_ext_id_copy', 'quota_and_usage')

    def cloud_id_copy(self, obj):
        return with_copy(obj.pk)

    cloud_id_copy.short_description = 'Cloud ID'

    def cloud_ext_id_copy(self, obj):
        return with_copy(obj.cloud_ext_id)

    cloud_ext_id_copy.short_description = 'Cloud Ext ID'

    def clusters_link(self, obj):
        if obj and obj.clusters_used:
            folders = ','.join([str(f['folder_id']) for f in obj.folders.all().values('folder_id')])
            return mark_safe("<a href=/meta/cluster/?folder_id__in=%s>%d&nbsp;</a>" % (folders, obj.clusters_used))

    clusters_link.short_description = 'Clusters'

    def get_inline_instances(self, request, obj=None):
        if obj is None or not obj.folders.exists():
            return []
        return super().get_inline_instances(request, obj)

    def quota_and_usage(self, obj):
        if obj:
            return loader.get_template('parts/cloud_quota_and_usage.html').render(
                {
                    'cpu_quota': obj.cpu_quota,
                    'cpu_used': obj.cpu_used,
                    'memory_quota': obj.memory_quota,
                    'memory_used': obj.memory_used,
                    'ssd_space_quota': obj.ssd_space_quota,
                    'ssd_space_used': obj.ssd_space_used,
                    'hdd_space_quota': obj.hdd_space_quota,
                    'hdd_space_used': obj.hdd_space_used,
                    'gpu_quota': obj.gpu_quota,
                    'gpu_used': obj.gpu_used,
                    'clusters_quota': obj.clusters_quota,
                    'clusters_used': obj.clusters_used,
                }
            )


class HostInline(ROTablularInline):
    fields = ('host_link', 'flavor_pretty', 'space_limit_pretty', 'vtype_id', 'geo_pretty', 'created_at')
    raw_id_fields = ('subcid', 'shard')
    model = models.Host
    classes = ['collapse']

    def host_link(self, obj):
        if obj:
            return obj.link

    host_link.short_description = 'FQDN'

    def space_limit_pretty(self, obj):
        if obj:
            return size_pretty(obj.space_limit)

    space_limit_pretty.short_description = 'Space Limit'

    def geo_pretty(self, obj):
        if obj and obj.geo_id:
            return _get_geo().get(obj.geo_id, obj.geo_id)

    geo_pretty.short_description = 'Geo'

    def flavor_pretty(self, obj):
        if obj and obj.flavor_id:
            return _get_flavors().get(obj.flavor_id, obj.flavor_id)

    flavor_pretty.short_description = 'Flavor'


class ShardInline(ROTablularInline):
    fields = ('shard_link', 'name', 'created_at')
    model = models.Shard
    ordering = ('created_at',)
    classes = ['collapse']

    def shard_link(self, obj):
        if obj:
            return obj.link

    shard_link.short_description = 'Shard'


class SubclustersInline(ROTablularInline):
    inlines = [ShardInline, HostInline]
    fields = ('subcid_link', 'cid', 'name', 'roles', 'created_at')
    model = models.Subcluster

    def subcid_link(self, obj):
        if obj and obj.subcid:
            return obj.link

    subcid_link.short_description = 'Subcluster'


class ClustersAdmin(ROAdmin):
    inlines = (SubclustersInline, MaintenanceTaskInline, WorkerQueueInline)
    list_display = ('cid', 'name', 'type', 'env', 'created_at', 'folder', 'status')
    fields = (
        'cid_copy',
        'folder_link',
        'name',
        'type',
        'env',
        'created_at',
        'status',
        'pretty_pillar',
        'pillar_revs',
        'graphics',
        'revs',
        'versions',
        'maint_window',
    )
    raw_id_fields = ('folder',)
    list_filter = ('type', 'env', 'status', VersionFilter)
    search_fields = ('cid', 'name')

    def cid_copy(self, obj):
        return with_copy(obj)

    cid_copy.short_description = 'CID'

    def folder_link(self, obj):
        if obj and obj.folder_id:
            return obj.folder.link

    folder_link.short_description = 'Folder'

    def pretty_pillar(self, obj):
        if obj and obj.cid:
            return mark_safe(render_pillar(models.Pillar.get_value(cid=obj.cid)))

    pretty_pillar.short_description = 'Pillar'

    def pillar_revs(self, obj):
        if obj and obj.cid:
            pillar_revs = models.PillarRev.objects.filter(cid=obj.cid).order_by('rev').values('rev', 'value')
            return mark_safe(render_pillar_revs(models.PillarRev.get_changes(pillar_revs[:MAX_SHOW_PILLAR_REVS])))

    def graphics(self, obj):
        if obj and obj.cid:
            links = get_cluster_links(obj.type, obj.cid, settings.INSTALLATION)
            if not links:
                return
            result = ['<table>']
            for k, v in links:
                result.append('<tr><td>{k}</td><td><a href="{v}" target="_blank">{v}</a></td></tr>'.format(k=k, v=v))
            result.append('</table>')
            return mark_safe(''.join(result))

    def revs(self, obj):
        if obj and obj.cid:
            return mark_safe(
                make_spoiler(
                    'Revisions here',
                    render_table(
                        ['Rev', 'Name', 'Network ID', 'Folder ID', 'Description', 'Status'],
                        [
                            [
                                row['rev'],
                                row['name'],
                                row['network_id'],
                                row['folder_id'],
                                row.get('description', ''),
                                row['status'],
                            ]
                            for row in obj.get_revs()
                        ],
                    ),
                )
            )

    def versions(self, obj):
        if obj and obj.cid:
            return mark_safe(
                render_table(
                    ['component', 'major_version', 'minor_version', 'package_version', 'edition'],
                    [
                        [
                            row['component'],
                            row['major_version'],
                            row['minor_version'],
                            row['package_version'],
                            row['edition'],
                        ]
                        for row in obj.get_versions()
                    ],
                )
            )

    def maint_window(self, obj):
        if obj and obj.cid:
            mw = obj.get_maint_window()
            if mw:
                return 'at %s o\'clock on %s' % (mw['hour'], mw['day'])


class SubclustersAdmin(ROAdmin):
    list_display = ('subcid', 'cid', 'name', 'roles', 'created_at')
    list_filter = ('roles',)
    fields = ('subcid_copy', 'cid_link', 'name', 'roles', 'created_at', 'pretty_pillar', 'pillar_revs')
    search_fields = ('subcid', 'cid__cid')
    raw_id_fields = ('cid',)

    def subcid_copy(self, obj):
        return with_copy(obj.pk)

    subcid_copy.short_description = 'SubCID'

    def cid_link(self, obj):
        if obj:
            return obj.cid.link

    cid_link.short_description = 'Cluster'

    def pretty_pillar(self, obj):
        if obj and obj.subcid:
            return mark_safe(render_pillar(models.Pillar.get_value(subcid=obj.subcid)))

    pretty_pillar.short_description = 'Pillar'

    def pillar_revs(self, obj):
        if obj and obj.subcid:
            pillar_revs = models.PillarRev.objects.filter(subcid=obj.subcid).order_by('rev').values('rev', 'value')
            return mark_safe(render_pillar_revs(models.PillarRev.get_changes(pillar_revs[:MAX_SHOW_PILLAR_REVS])))

    def get_inline_instances(self, request, obj=None):
        inlines = []
        if obj:
            if models.Shard.objects.filter(subcid=obj).exists():
                inlines = [ShardInline, HostInline]
            else:
                inlines = [HostInline]
        self.inlines = inlines
        return super().get_inline_instances(request, obj)


class FlavorAdmin(ROAdmin):
    list_display = (
        'id',
        'name',
        'cpu_guarantee',
        'cpu_limit',
        'memory_guarantee',
        'memory_limit',
        'visible',
        'vtype',
        'type',
    )
    list_filter = ('visible', 'type', 'generation', 'vtype')
    fields = (
        'id',
        'name',
        'cpu_guarantee',
        'cpu_limit',
        'memory_guarantee',
        'memory_limit',
        'network_guarantee',
        'network_limit',
        'io_limit',
        'visible',
        'vtype',
        'platform_id',
        'generation',
        'type',
    )


class HostAdmin(ROAdmin):
    list_display = ('fqdn', 'subcid', 'cid', 'flavor', 'space_limit_pretty', 'geo', 'created_at')
    raw_id_fields = ('subcid', 'shard')
    search_fields = ('fqdn', 'subcid__subcid', 'subcid__cid__cid')
    list_filter = ('geo',)
    fields = (
        'fqdn_copy',
        'subcid_link',
        'cid_link',
        'shard',
        'flavor',
        'space_limit_pretty',
        'vtype_id',
        'geo',
        'disk_type',
        'subnet_id',
        'created_at',
        'pretty_pillar',
        'pillar_revs',
        'revs',
        'graphics',
        'cms_last_decision',
        'links',
    )

    def cid(self, obj):
        if obj and obj.subcid:
            return obj.subcid.cid

    cid.short_description = 'CID'

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def cid_link(self, obj):
        if obj and obj.subcid:
            return obj.subcid.cid.link

    cid_link.short_description = 'Cluster'

    def subcid_link(self, obj):
        if obj and obj.subcid:
            return obj.subcid.link

    subcid_link.short_description = 'Subcluster'

    def space_limit_pretty(self, obj):
        if obj:
            return size_pretty(obj.space_limit)

    space_limit_pretty.short_description = 'Space Limit'

    def pretty_pillar(self, obj):
        if obj and obj.fqdn:
            return mark_safe(render_pillar(models.Pillar.get_value(fqdn=obj.fqdn)))

    pretty_pillar.short_description = 'Pillar'

    def pillar_revs(self, obj):
        if obj and obj.fqdn:
            pillar_revs = models.PillarRev.objects.filter(fqdn=obj.fqdn).order_by('rev').values('rev', 'value')
            return mark_safe(render_pillar_revs(models.PillarRev.get_changes(pillar_revs[:MAX_SHOW_PILLAR_REVS])))

    def revs(self, obj):
        if obj and obj.fqdn:
            return mark_safe(
                make_spoiler(
                    'Revisions here',
                    render_table(
                        [
                            'Rev',
                            'Flavor',
                            'Space Limit',
                            'vtype_id',
                            'Geo',
                            'Disk Type',
                            'Subnet ID',
                            'Assign Public IP',
                            'Сreated at',
                        ],
                        [
                            [
                                row['rev'],
                                row['flavor'],
                                size_pretty(row['space_limit']),
                                row['vtype_id'],
                                row['geo'],
                                row['disk_type'],
                                row['subnet_id'],
                                row['assign_public_ip'],
                                row['created_at'],
                            ]
                            for row in obj.get_revs()
                        ],
                    ),
                )
            )

    def links(self, obj):
        if obj:
            return mark_safe('<br />'.join(search_by_fqdn(obj.fqdn)))

    links.short_description = 'Related Links'

    def graphics(self, obj):
        if obj:
            links = get_host_links(obj.fqdn, obj.subcid.cid, settings.INSTALLATION)
            if not links:
                return
            result = ['<table>']
            for k, v in links:
                result.append('<tr><td>{k}</td><td><a href="{v}" target="_blank">{v}</a></td></tr>'.format(k=k, v=v))
            result.append('</table>')
            return mark_safe(''.join(result))

    def cms_last_decision(self, obj):
        if settings.ENABLED_APPS.cms:
            return '-'
        if obj:
            try:
                decision = get_cms_last_decision(obj.fqdn)
            except Exception:
                return '-'
            if decision:
                return mark_safe(
                    '<br />'.join(
                        [
                            '<a href="/cms/decision/{id}/">Decision <b>{id}</b> at {created}</a>'.format(
                                id=decision.id, created=decision.request.created_at.strftime('%d.%m.%Y %H:%M')
                            ),
                            walle_link(decision),
                        ]
                    )
                )
        return '-'


def _has_changes(old, new, pillar):
    if pillar:
        return True
    if not old and not new:
        return False
    if type(old) == dict and type(new) == dict:
        for k, v in old.items():
            if k not in {'rev', 'created_at'} and v != new.get(k):
                return True
        return False
    return True


class WorkerQueueAdmin(DjangoObjectActions, ROAdmin):
    list_display = ('task_id', 'cid', 'create_ts', 'end_ts', 'task_type', 'result', 'worker_id')
    fields = (
        'task_id_copy',
        'worker_id_copy',
        'cid_link',
        'create_ts',
        'start_ts',
        'end_ts',
        'task_type',
        'task_args',
        'result',
        'restart_count',
        'changes_pretty',
        'comment',
        'created_by',
        'folder_link',
        'operation_type',
        'metadata',
        'hidden',
        'version',
        'delayed_until',
        'required_task',
        'errors_pretty',
        'context',
        'timeout',
        'create_rev',
        'acquire_rev',
        'finish_rev',
        'unmanaged',
        'tracing',
        'jaeger_link',
        'target_rev',
        'config_id',
        'shipments_table',
        'acquire_finish_changes',
    )
    raw_id_fields = ('cid', 'folder', 'required_task')
    list_filter = ('result', 'cid__type', 'cid__status')
    model = models.WorkerQueue
    change_actions = [
        'restart_task_action_single',
    ]

    def get_change_actions(self, request, object_id, form_url):
        obj = models.WorkerQueue.objects.filter(task_id=object_id).first()
        if obj and logic.can_restart_task(obj):
            return [
                'restart_task_action_single',
            ]
        else:
            return []

    def restart_task_action_single(self, request, obj):
        print('Task %s restarted by %s' % (obj.task_id, request.user.username))
        return logic.restart_task(obj.task_id)

    restart_task_action_single.label = 'Restart'
    restart_task_action_single.short_description = 'Restart task'

    def changes_pretty(self, obj):
        if obj and obj.changes:
            result = []
            for ch in obj.changes:
                timestamp = ch['timestamp']
                for k, v in ch.items():
                    if k != 'timestamp':
                        result.append('{ts} {k}: {v}'.format(ts=timestamp, k=k, v=v))
            return mark_safe('<pre>' + '\n'.join(result) + '</pre>')

    changes_pretty.short_description = 'Changes'

    def errors_pretty(self, obj):
        if obj:
            return mark_safe('<pre class="json">' + json.dumps(obj.errors) + '</pre>')

    errors_pretty.short_description = 'Errors'

    def task_id_copy(self, obj):
        return with_copy(obj.task_id)

    task_id_copy.short_description = 'Task ID'

    def worker_id_copy(self, obj):
        return with_copy(obj.worker_id)

    worker_id_copy.short_description = 'Worker ID'

    def cid_link(self, obj):
        return obj.cid.link

    cid_link.short_description = 'CID'

    def folder_link(self, obj):
        if obj and obj.folder_id:
            return obj.folder.link

    folder_link.short_description = 'Folder'

    def jaeger_link(self, obj):
        if obj and obj.tracing:
            return mark_safe(make_jaeger_link(obj.tracing, settings.INSTALLATION))

    def acquire_finish_changes(self, obj):
        if obj and obj.acquire_rev and obj.finish_rev:
            cluster = obj.cid

            data = [
                (
                    'Cluster',
                    list(
                        filter(
                            lambda x: _has_changes(x[1], x[2], x[3]),
                            [
                                (
                                    cluster.cid,
                                    cluster.get_prev_rev(obj.create_rev - 1),
                                    cluster.get_prev_rev(obj.finish_rev),
                                    '<br />'.join(
                                        [
                                            render_change(change)
                                            for change in models.PillarRev.get_changes(
                                                [
                                                    models.PillarRev.get_prev_rev(obj.create_rev - 1, cid=cluster.cid),
                                                    models.PillarRev.get_prev_rev(obj.finish_rev, cid=cluster.cid),
                                                ],
                                                with_revs=False,
                                            )
                                        ]
                                    ),
                                )
                            ],
                        )
                    ),
                ),
                (
                    'Subclusters',
                    list(
                        filter(
                            lambda x: _has_changes(x[1], x[2], x[3]),
                            [
                                (
                                    subcid,
                                    models.Subcluster.get_prev_rev(subcid, obj.create_rev - 1),
                                    models.Subcluster.get_prev_rev(subcid, obj.finish_rev),
                                    '<br />'.join(
                                        [
                                            render_change(change)
                                            for change in models.PillarRev.get_changes(
                                                [
                                                    models.PillarRev.get_prev_rev(obj.create_rev - 1, subcid=subcid),
                                                    models.PillarRev.get_prev_rev(obj.finish_rev, subcid=subcid),
                                                ],
                                                with_revs=False,
                                            )
                                        ]
                                    ),
                                )
                                for subcid in cluster.get_historical_subclusters()
                            ],
                        )
                    ),
                ),
                (
                    'Hosts',
                    list(
                        filter(
                            lambda x: _has_changes(x[1], x[2], x[3]),
                            [
                                (
                                    fqdn,
                                    models.Host.get_prev_rev(fqdn, obj.create_rev - 1),
                                    models.Host.get_prev_rev(fqdn, obj.finish_rev),
                                    '<br />'.join(
                                        [
                                            render_change(change)
                                            for change in models.PillarRev.get_changes(
                                                [
                                                    models.PillarRev.get_prev_rev(obj.create_rev - 1, fqdn=fqdn),
                                                    models.PillarRev.get_prev_rev(obj.finish_rev, fqdn=fqdn),
                                                ],
                                                with_revs=False,
                                            )
                                        ]
                                    ),
                                )
                                for fqdn in cluster.get_historical_hosts()
                            ],
                        )
                    ),
                ),
            ]

            return loader.get_template('parts/worker_queue_acquire_finish_changes.html').render({'data': data})

    def shipments_table(self, obj):
        try:
            shipment_ids = obj.parse_shipments()
        except Exception:
            shipment_ids = []
        if not shipment_ids:
            return 'Shipments not found'
        shipment_commands = ShipmentCommand.objects.filter(shipment_id__in=shipment_ids)
        commands = Command.objects.filter(shipment_command_id__in=shipment_commands)
        jobs = Job.objects.filter(command__in=commands).order_by('-created_at')

        return mark_safe(
            render_table(
                [
                    'Shipment Id',
                    'Shipment Command',
                    'FQDN',
                    'Job Id',
                    'Ext Job Id',
                    'Status',
                    'Created at',
                    'Last Running Check at',
                    'Failed States',
                ],
                [
                    [
                        "<a href='/deploy/shipment/{_id}/'>{_id}</a>".format(
                            _id=j.command.shipment_command.shipment_id
                        ),
                        "<a href='/deploy/command/%s/'>%s</a>"
                        % (j.command.shipment_command_id, j.command.shipment_command.type),
                        j.command.minion.fqdn,
                        j.job_id,
                        get_job_id_link(j.ext_job_id),
                        j.status,
                        j.created_at,
                        j.last_running_check_at,
                        ', '.join(j.failed_states),
                    ]
                    for j in jobs
                ],
            )
        )


class KnownFeatureFlags(Enum):
    ALLOW_DECOMMISSIONED_ZONE_USE = 'MDB_ALLOW_DECOMMISSIONED_ZONE_USE'
    CLICKHOUSE_FAST_OPS = 'MDB_CLICKHOUSE_FAST_OPS'
    CLICKHOUSE_SHARDING = 'MDB_CLICKHOUSE_SHARDING'
    CLICKHOUSE_UNLIMITED_SHARD_COUNT = 'MDB_CLICKHOUSE_UNLIMITED_SHARD_COUNT'
    CLICKHOUSE_UPGRADE = 'MDB_CLICKHOUSE_UPGRADE'
    CLICKHOUSE_CLOUD_STORAGE = 'MDB_CLICKHOUSE_CLOUD_STORAGE'
    CLICKHOUSE_CLOUD_STORAGE_HA = 'MDB_CLICKHOUSE_CLOUD_STORAGE_HA'
    CLICKHOUSE_SQL_MANAGEMENT = 'MDB_CLICKHOUSE_SQL_MANAGEMENT'
    CLICKHOUSE_TESTING_VERSIONS = 'MDB_CLICKHOUSE_TESTING_VERSIONS'
    CLICKHOUSE_DISABLE_CLUSTER_CONFIGURATION_CHECKS = 'MDB_CLICKHOUSE_DISABLE_CLUSTER_CONFIGURATION_CHECKS'
    CLICKHOUSE_ZK_ON_NETWORK_HDD = 'MDB_CLICKHOUSE_ZK_ON_NETWORK_HDD'
    DATAPROC_INSTANCE_GROUPS = 'MDB_DATAPROC_INSTANCE_GROUPS'
    DATAPROC_MANAGER = 'MDB_DATAPROC_MANAGER'
    DATAPROC_AUTOSCALING = 'MDB_DATAPROC_AUTOSCALING'
    HADOOP_ALPHA = 'MDB_HADOOP_ALPHA'
    HADOOP_GPU = 'MDB_HADOOP_GPU'
    LOCAL_DISK_RESIZE = 'MDB_LOCAL_DISK_RESIZE'
    MONGODB_40 = 'MDB_MONGODB_40'
    MONGODB_4_2 = 'MDB_MONGODB_4_2'
    MONGODB_4_2_RS_UPGRADE = 'MDB_MONGODB_4_2_RS_UPGRADE'
    MONGODB_4_2_SHARDED_UPGRADE = 'MDB_MONGODB_4_2_SHARDED_UPGRADE'
    MONGODB_EXTENDEDS = 'MDB_MONGODB_EXTENDEDS'
    MONGODB_FAST_OPS = 'MDB_MONGODB_FAST_OPS'
    MONGODB_UNLIMITED_SHARD_COUNT = 'MDB_MONGODB_UNLIMITED_SHARD_COUNT'
    MYSQL_80 = 'MDB_MYSQL_80'
    MYSQL_FAST_OPS = 'MDB_MYSQL_FAST_OPS'
    NETWORK_DISK_NO_STOP_RESIZE = 'MDB_NETWORK_DISK_NO_STOP_RESIZE'
    NETWORK_DISK_TRUNCATE = 'MDB_NETWORK_DISK_TRUNCATE'
    POSTGRESQL_10_1C = 'MDB_POSTGRESQL_10_1C'
    POSTGRESQL_11 = 'MDB_POSTGRESQL_11'
    POSTGRESQL_11_1C = 'MDB_POSTGRESQL_11_1C'
    POSTGRESQL_12_1C = 'MDB_POSTGRESQL_12_1C'
    POSTGRESQL_13 = 'MDB_POSTGRESQL_13'
    POSTGRESQL_FAST_OPS = 'MDB_POSTGRESQL_FAST_OPS'
    REDIS = 'MDB_REDIS'
    REDIS_FAST_OPS = 'MDB_REDIS_FAST_OPS'
    REDIS_SHARDING = 'MDB_REDIS_SHARDING'
    KAFKA_CLUSTER = 'MDB_KAFKA_CLUSTER'
    SQLSERVER_CLUSTER = 'MDB_SQLSERVER_CLUSTER'
    ELASTICSEARCH_CLUSTER = 'MDB_ELASTICSEARCH_CLUSTER'
    DATAPROC_UI_PROXY = 'MDB_DATAPROC_UI_PROXY'
    MONGODB_RS_PITR = 'MDB_MONGODB_RS_PITR'
    ALLOW_NETWORK_SSD_NONREPLICATED = 'MDB_ALLOW_NETWORK_SSD_NONREPLICATED'
    DATAPROC_IMAGE_1_3 = 'MDB_DATAPROC_IMAGE_1_3'
    MONGODB_INFRA_CFG = 'MDB_MONGODB_INFRA_CFG'
    MONGODB_4_4 = 'MDB_MONGODB_4_4'
    MONGODB_4_4_RS_UPGRADE = 'MDB_MONGODB_4_4_RS_UPGRADE'
    MONGODB_4_4_SHARDED_UPGRADE = 'MDB_MONGODB_4_4_SHARDED_UPGRADE'
    MONGODB_BACKUP_SERVICE = 'MDB_MONGODB_BACKUP_SERVICE'
    FORCE_UNSAFE_RESIZE = 'MDB_FORCE_UNSAFE_RESIZE'
    MONGODB_RESTORE_WITHOUT_REPLAY = 'MDB_MONGODB_RESTORE_WITHOUT_REPLAY'
    FLAVOR_80_512 = 'MDB_FLAVOR_80_512'
    DATAPROC_IMAGE_2_0 = 'MDB_DATAPROC_IMAGE_2_0'
    MONGODB_ALLOW_DEPRECATED_VERSIONS = 'MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS'
    MONGODB_PERF_DIAG = 'MDB_MONGODB_PERF_DIAG'
    MONGODB_SHARDED_PITR = 'MDB_MONGODB_SHARDED_PITR'


class FeatureFlagFilter(admin.SimpleListFilter):
    title = 'Feature flag'
    parameter_name = 'feature_flag'

    def lookups(self, request, model_admin):
        return [[item.name, item.value] for item in KnownFeatureFlags]

    def queryset(self, request, queryset):
        value = self.value()
        if value is None:
            return queryset
        try:
            feature_flag_name = KnownFeatureFlags(value).value
        except ValueError:
            raise RuntimeError('unknown value {}'.format(value))
        else:
            return queryset.filter(feature_flag__contains=feature_flag_name)


class ValidResourcesAdmin(ROAdmin):
    list_display = ('id', 'cluster_type', 'role', 'flavor', 'disk_type', 'geo', 'feature_flag')
    list_filter = (
        'cluster_type',
        'role',
        'flavor',
        'disk_type',
        'geo',
        FeatureFlagFilter,
    )
    fields = (
        'id',
        'cluster_type',
        'role',
        'flavor',
        'disk_type',
        'disk_size_range',
        'min_hosts',
        'max_hosts',
        'geo',
        'feature_flag',
    )


class FolderAdmin(ROAdmin):
    inlines = (ClustersInline,)
    list_display = ('folder_id', 'folder_ext_id', 'cloud')
    fields = ('folder_id_copy', 'folder_ext_id_copy', 'cloud_link')
    search_fields = ('folder_id', 'folder_ext_id')

    def folder_id_copy(self, obj):
        return with_copy(obj.folder_id)

    folder_id_copy.short_description = 'Folder ID'

    def folder_ext_id_copy(self, obj):
        return with_copy(obj.folder_ext_id)

    folder_ext_id_copy.short_description = 'Folder Ext ID'

    def cloud_link(self, obj):
        if obj and obj.cloud:
            return obj.cloud.link

    cloud_link.short_description = 'Cloud'


class ShardAdmin(ROAdmin):
    inlines = (HostInline,)
    list_display = ('shard_id', 'subcid', 'name', 'created_at')
    fields = ('subcid_link', 'shard_id', 'name', 'created_at', 'pretty_pillar', 'pillar_revs')
    raw_id_fields = ('subcid',)

    def subcid_link(self, obj):
        if obj and obj.subcid:
            return obj.subcid.link

    subcid_link.short_description = 'Subcluster'

    def pretty_pillar(self, obj):
        if obj and obj.shard_id:
            return mark_safe(render_pillar(models.Pillar.get_value(shard_id=obj.shard_id)))

    pretty_pillar.short_description = 'Pillar'

    def pillar_revs(self, obj):
        if obj and obj.shard_id:
            pillar_revs = models.PillarRev.objects.filter(shard_id=obj.shard_id).order_by('rev').values('rev', 'value')
            return mark_safe(render_pillar_revs(models.PillarRev.get_changes(pillar_revs[:MAX_SHOW_PILLAR_REVS])))


class MaintenanceTasksAdmin(ROAdmin):
    actions = None
    list_display = ('task_id', 'cid_link', 'config_id', 'create_ts', 'plan_ts', 'status', 'info')
    list_display_links = None
    fields = ('cid_link', 'config_id', 'task_id', 'create_ts', 'plan_ts', 'status', 'info')
    list_filter = ('status', 'cid__env')
    search_fields = ('cid',)

    def cid_link(self, obj):
        if obj:
            return obj.cid.link

    cid_link.short_description = 'Cluster'


admin.site.register(models.Cloud, CloudsAdmin)
admin.site.register(models.Cluster, ClustersAdmin)
admin.site.register(models.Subcluster, SubclustersAdmin)
admin.site.register(models.Flavor, FlavorAdmin)
admin.site.register(models.Host, HostAdmin)
admin.site.register(models.WorkerQueue, WorkerQueueAdmin)
admin.site.register(models.ValidResource, ValidResourcesAdmin)
admin.site.register(models.Folder, FolderAdmin)
admin.site.register(models.Shard, ShardAdmin)
admin.site.register(models.MaintenanceTask, MaintenanceTasksAdmin)
