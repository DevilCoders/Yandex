from cloud.mdb.ui.internal.dbm import models

from django.contrib import admin
from django.template import loader
from django.utils.safestring import mark_safe

from cloud.mdb.ui.internal.common import ROAdmin, ROTablularInline, size_pretty, with_copy
from cloud.mdb.ui.internal.omnisearch import search_by_fqdn


class VolumeInline(ROTablularInline):
    model = models.Volume
    fields = ('path', 'space_guarantee', 'space_limit_pretty', 'dom0_path', 'backend', 'read_only', 'disk')

    def space_limit_pretty(self, obj):
        if obj:
            return size_pretty(obj.space_limit)

    space_limit_pretty.short_description = 'Space Limit'


class Dom0HostInline(ROTablularInline):
    model = models.Dom0Host
    fields = (
        'fqdn_link',
        'project',
        'geo',
        'generation',
        'cpu_cores',
        'memory',
        'ssd_space',
        'sata_space',
        'max_io',
        'net_speed',
        'allow_new_hosts',
        'allow_new_hosts_updated_by',
        'heartbeat',
        'use_vlan688',
        'switch',
    )

    def fqdn_link(self, obj):
        if obj:
            return obj.link

    fqdn_link.short_description = 'fqdn'


class ContainerInline(ROTablularInline):
    model = models.Container
    fields = (
        'fqdn_link',
        'dom0_link',
        'cluster_link',
        'generation',
        'cpu_limit',
        'memory_limit_pretty',
        'net_limit_pretty',
        'io_limit_pretty',
        'bootstrap_cmd',
    )

    def dom0_link(self, obj):
        if obj and obj.dom0:
            return obj.dom0.link

    dom0_link.short_description = 'Dom0'

    def fqdn_link(self, obj):
        if obj:
            return obj.link

    fqdn_link.short_description = 'Container'

    def cluster_link(self, obj):
        if obj:
            return obj.cluster_name.link

    cluster_link.short_description = 'Cluster'

    def memory_limit_pretty(self, obj):
        if obj:
            return size_pretty(obj.memory_limit)

    memory_limit_pretty.short_description = 'Memory Limit'

    def net_limit_pretty(self, obj):
        if obj:
            return size_pretty(obj.net_limit)

    net_limit_pretty.short_description = 'Net Limit'

    def io_limit_pretty(self, obj):
        if obj:
            return size_pretty(obj.io_limit)

    io_limit_pretty.short_description = 'IO Limit'


class ClustersAdmin(ROAdmin):
    inlines = (ContainerInline,)
    list_display = ('name', 'project')
    fields = ('name_copy', 'project')
    list_filter = ('project',)
    search_fields = ('name',)

    def name_copy(self, obj):
        return with_copy(obj.name)

    name_copy.short_description = 'Name'


class ContainersAdmin(ROAdmin):
    inlines = (VolumeInline,)
    list_display = ('fqdn', 'dom0', 'cluster_name', 'generation')
    fields = (
        'fqdn_copy',
        'dom0_link',
        'cluster_link',
        'generation',
        'cpu_guarantee',
        'cpu_limit',
        'memory_guarantee',
        'memory_limit',
        'hugetlb_limit',
        'net_guarantee',
        'net_limit',
        'io_limit',
        'extra_properties',
        'bootstrap_cmd',
        'secrets',
        'secrets_expire',
        'pending_delete',
        'delete_token',
        'links',
    )
    list_filter = ('generation',)
    search_fields = ('fqdn',)

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def dom0_link(self, obj):
        if obj:
            return obj.dom0.link

    dom0_link.short_description = 'Dom0'

    def cluster_link(self, obj):
        if obj:
            return obj.cluster_name.link

    cluster_link.short_description = 'Cluster'

    def links(self, obj):
        if obj:
            return mark_safe('<br />'.join(search_by_fqdn(obj.fqdn)))

    links.short_description = 'Related Links'


class Dom0HostsAdmin(ROAdmin):
    inlines = (ContainerInline,)
    list_display = ('fqdn', 'project', 'geo', 'generation')
    fields = (
        'fqdn_copy',
        'project',
        'geo',
        'generation',
        'allow_new_hosts',
        'allow_new_hosts_updated_by',
        'heartbeat',
        'use_vlan688',
        'switch',
        'links',
        'resources',
    )
    list_filter = ('project', 'geo', 'generation')
    search_fields = ('fqdn',)

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def links(self, obj):
        if obj:
            return mark_safe('<br />'.join(search_by_fqdn(obj.fqdn)))

    links.short_description = 'Related Links'

    def resources(self, obj):
        used = obj.resources_used
        data = {
            'cpu': [obj.cpu_cores, used['cpu'], obj.cpu_cores - used['cpu']],
            'memory': [size_pretty(obj.memory), size_pretty(used['memory']), size_pretty(obj.memory - used['memory'])],
            'net': [size_pretty(obj.net_speed), size_pretty(used['net']), size_pretty(obj.net_speed - used['net'])],
            'io': [size_pretty(obj.max_io), size_pretty(used['io']), size_pretty(obj.max_io - used['io'])],
            'ssd': [size_pretty(obj.ssd_space), size_pretty(used['ssd']), size_pretty(obj.ssd_space - used['ssd'])],
            'sata': [
                size_pretty(obj.sata_space),
                size_pretty(used['sata']),
                size_pretty(obj.sata_space - used['sata']),
            ],
        }
        return loader.get_template('parts/dom0_resources.html').render(data)


class ProjectsAdmin(ROAdmin):
    inlines = (Dom0HostInline,)
    list_display = ('name', 'description')
    fields = ('name', 'description')


class TransfersAdmin(ROAdmin):
    list_display = ('id', 'src_dom0', 'dest_dom0', 'container', 'started')
    fields = ('id', 'src_dom0', 'dest_dom0', 'container', 'started', 'placeholder')


class ReservedResourcesAdmin(ROAdmin):
    list_display = ('generation', 'cpu_cores', 'memory', 'io', 'net', 'ssd_space')
    fields = ('generation', 'cpu_cores', 'memory', 'io', 'net', 'ssd_space')
    ordering = ('generation',)


admin.site.register(models.Cluster, ClustersAdmin)
admin.site.register(models.Container, ContainersAdmin)
admin.site.register(models.Dom0Host, Dom0HostsAdmin)
admin.site.register(models.Project, ProjectsAdmin)
admin.site.register(models.Transfer, TransfersAdmin)
admin.site.register(models.ReservedResource, ReservedResourcesAdmin)
