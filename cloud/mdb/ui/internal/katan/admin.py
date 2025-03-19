from django.contrib import admin
from django.utils.safestring import mark_safe

from cloud.mdb.ui.internal.common import ROAdmin, ROTablularInline, render_table, with_copy
from cloud.mdb.ui.internal.deploy.models import Shipment
from cloud.mdb.ui.internal.katan import models
from cloud.mdb.ui.internal.omnisearch import search_by_fqdn
from django_admin_inline_paginator.admin import PaginationFormSetBase


class HostsInline(ROTablularInline):
    model = models.Host
    fields = ('fqdn_link', 'cluster', 'tags')

    def fqdn_link(self, obj):
        return obj.link

    fqdn_link.short_description = 'FQDN'


class RolloutInline(ROTablularInline):
    model = models.Rollout
    fields = ('rollout_link', 'commands', 'started_at', 'finished_at')
    max_num = 5
    ordering = ('-started_at',)
    template = 'admin/tabular_paginated.html'
    pagination_key = 'page'
    per_page = 10

    def rollout_link(self, obj):
        return obj.link

    rollout_link.short_description = 'Rollout ID'

    def get_formset(self, request, obj=None, **kwargs):
        formset_class = super().get_formset(request, obj, **kwargs)

        class PaginationFormSet(PaginationFormSetBase, formset_class):
            pagination_key = self.pagination_key

        PaginationFormSet.request = request
        PaginationFormSet.per_page = self.per_page
        return PaginationFormSet


def make_cluster_rollouts_table(**kwargs):
    return mark_safe(
        render_table(
            ['rollout', 'cluster', 'state', 'updated_at', 'comment'],
            [
                [
                    models.Rollout.objects.get(rollout_id=row['rollout']).link,
                    models.Cluster.objects.get(cluster_id=row['cluster']).link,
                    row['state'],
                    row['updated_at'],
                    row['comment'],
                ]
                for row in models.ClusterRollout.objects.filter(**kwargs)
                .values('rollout', 'cluster', 'state', 'updated_at', 'comment')
                .order_by('-rollout_id')
            ],
        )
    )


def make_rollout_shipments_table(**kwargs):
    return mark_safe(
        render_table(
            ['rollout', 'fqdn', 'shipment_id'],
            [
                [
                    models.Rollout.objects.get(rollout_id=row['rollout']).link,
                    models.Host.objects.get(fqdn=row['fqdn']).link,
                    Shipment.objects.get(shipment_id=row['shipment_id']).link,
                ]
                for row in models.RolloutShipment.objects.filter(**kwargs).values('rollout', 'fqdn', 'shipment_id')
            ],
        )
    )


class ClusterAdmin(ROAdmin):
    inlines = (HostsInline,)
    list_display = ('cluster_id', 'tags', 'imported_at', 'auto_update')
    fields = ('cluster_id_copy', 'tags', 'imported_at', 'auto_update', 'cluster_rollouts')

    def cluster_id_copy(self, obj):
        return with_copy(obj)

    cluster_id_copy.short_description = 'Cluster ID'

    def cluster_rollouts(self, obj):
        if obj:
            return make_cluster_rollouts_table(cluster=obj)


class HostAdmin(ROAdmin):
    list_display = ('fqdn', 'cluster', 'tags')
    fields = ('fqdn_copy', 'cluster_link', 'tags', 'shipments', 'links')
    search_fields = (
        'fqdn',
        'cluster__cluster_id',
    )

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def shipments(self, obj):
        if obj:
            return make_rollout_shipments_table(fqdn=obj.fqdn)

    def cluster_link(self, obj):
        if obj:
            return obj.cluster.link

    cluster_link.short_description = 'Cluster'

    def links(self, obj):
        if obj:
            return mark_safe('<br />'.join(search_by_fqdn(obj.fqdn)))

    links.short_description = 'Related Links'


class RolloutAdmin(ROAdmin):
    list_display = ('rollout_id', 'commands', 'parallel', 'created_at', 'started_at', 'schedule')
    fields = (
        'rollout_id',
        'commands',
        'parallel',
        'created_at',
        'started_at',
        'finished_at',
        'created_by',
        'schedule',
        'comment',
        'cluster_rollouts',
        'dependencies',
        'shipments',
    )

    def cluster_rollouts(self, obj):
        if obj:
            return make_cluster_rollouts_table(rollout=obj)

    def dependencies(self, obj):
        if obj:
            return mark_safe(
                render_table(
                    ['depends_on'],
                    [
                        [row['depends_on']]
                        for row in models.RolloutsDependency.objects.filter(rollout=obj).values('depends_on')
                    ],
                )
            )

    def shipments(self, obj):
        if obj:
            return make_rollout_shipments_table(rollout=obj)


class ScheduleAdmin(ROAdmin):
    inlines = (RolloutInline,)
    list_display = ('schedule_id', 'namespace', 'match_tags', 'state', 'parallel', 'edited_at', 'edited_by')
    fields = (
        'schedule_id',
        'match_tags',
        'commands',
        'state',
        'age',
        'still_age',
        'max_size',
        'parallel',
        'edited_at',
        'edited_by',
        'examined_rollout_id',
        'namespace',
        'name',
        'dependencies',
        'convergence',
    )
    list_filter = ('state', 'namespace')

    def dependencies(self, obj):
        if obj:
            return mark_safe(
                render_table(
                    ['depends_on'],
                    [
                        [row['depends_on']]
                        for row in models.ScheduleDependency.objects.filter(schedule=obj).values('depends_on')
                    ],
                )
            )

    def convergence(self, obj):
        if obj:
            result_table = []
            for row in obj.get_convergence():
                result_table.append(
                    (
                        row[0].days if row[0] else 'null',
                        row[1].days if row[1] else 'null',
                        row[2],
                        ', '.join(
                            [
                                '<a href="/katan/cluster/{cid}/" target="_blank">{cid}</a>'.format(cid=cid)
                                for cid in row[3]
                            ]
                        )
                        if row[3]
                        else '-',
                    )
                )
            return mark_safe(
                render_table(['min_age (days)', 'max_age (days)', 'count', 'sample clusters'], result_table)
            )


admin.site.register(models.Cluster, ClusterAdmin)
admin.site.register(models.Host, HostAdmin)
admin.site.register(models.Rollout, RolloutAdmin)
admin.site.register(models.Schedule, ScheduleAdmin)
