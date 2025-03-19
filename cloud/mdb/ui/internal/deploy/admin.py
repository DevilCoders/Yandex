import json

from django.conf import settings
from django.contrib import admin
from django.utils.safestring import mark_safe

from cloud.mdb.ui.internal.common import (
    ROAdmin,
    ROStackedInline,
    ROTablularInline,
    make_jaeger_link,
    render_job_result,
    render_table,
    with_copy,
)
from cloud.mdb.ui.internal.deploy import models
from cloud.mdb.ui.internal.omnisearch import search_by_fqdn


def link(model_name, id, value):
    return mark_safe('<a href=/deploy/%s/%s/>%s</a>' % (model_name, id, value))


class GroupAdmin(ROAdmin):
    list_display = ('group_id', 'name', 'masters_link')
    fields = ('group_id_copy', 'name_copy', 'masters_link', 'minions_link')
    search_fields = ('name',)
    ordering = ('group_id',)

    def group_id_copy(self, obj):
        return with_copy(obj.group_id)

    group_id_copy.short_description = 'Group ID'

    def name_copy(self, obj):
        return with_copy(obj.name)

    name_copy.short_description = 'Name'

    def masters_link(self, obj):
        if obj:
            masters = models.Master.objects.filter(group=obj)
            return mark_safe(
                '<a href=/deploy/master/?group__group_id__exact=%d>%s</a>' % (obj.group_id, masters.count())
            )

    masters_link.short_description = 'Masters'

    def minions_link(self, obj):
        if obj:
            minions = models.Minion.objects.filter(group=obj)
            return mark_safe(
                '<a href=/deploy/minion/?group__group_id__exact=%d>%s</a>' % (obj.group_id, minions.count())
            )

    minions_link.short_description = 'Minions'


class MasterAdmin(ROAdmin):
    list_display = ('fqdn', 'group', 'is_open', 'is_alive', 'created_at')
    fields = ('fqdn_copy', 'group_link', 'minions_link', 'is_open', 'is_alive', 'created_at', 'links')
    list_filter = ('is_open', 'is_alive', 'group')
    search_fields = ('fqdn',)

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def group_link(self, obj):
        if obj and obj.group_id:
            return obj.group.link

    group_link.short_description = 'Group'

    def minions_link(self, obj):
        if obj:
            minions = models.Minion.objects.filter(master=obj)
            return mark_safe(
                '<a href=/deploy/minion/?master__master_id__exact=%d>%s</a>' % (obj.master_id, minions.count())
            )

    minions_link.short_description = 'Minions'

    def links(self, obj):
        if obj:
            return mark_safe('<br />'.join(search_by_fqdn(obj.fqdn)))

    links.short_description = 'Related Links'


class MinionAdmin(ROAdmin):
    list_display = ('fqdn', 'master', 'group', 'auto_reassign', 'deleted', 'created_at', 'commands_link')
    fields = ('fqdn_copy', 'master_link', 'group_link', 'auto_reassign', 'deleted', 'created_at', 'links')
    raw_id_fields = ('master',)
    list_filter = ('deleted', 'group__name')
    search_fields = ('fqdn',)

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def master_link(self, obj):
        if obj and obj.master:
            return obj.master.link

    master_link.short_description = 'Master'

    def group_link(self, obj):
        if obj and obj.group_id:
            return obj.group.link

    group_link.short_description = 'Group'

    def commands_link(self, obj):
        if obj and obj.minion_id:
            return mark_safe("<a href=/deploy/command/?minion_id=%d>&nbsp;Commands&nbsp;&rarr;</a>" % obj.minion_id)

    def links(self, obj):
        if obj:
            return mark_safe('<br />'.join(search_by_fqdn(obj.fqdn)))

    links.short_description = 'Related Links'


def get_job_id_link(ext_job_id):
    job_results = models.JobResult.objects.filter(ext_job_id=ext_job_id)
    if job_results.exists():
        return '<br />'.join(
            [
                with_copy(ext_job_id, link('jobresult', jr.job_result_id, '%s (%s)' % (ext_job_id, jr.status)))
                for jr in job_results.order_by('recorded_at')
            ]
        )
    else:
        return ext_job_id


class JobInline(ROTablularInline):
    model = models.Job
    fields = ('job_id', 'ext_job_id_link', 'command', 'status', 'created_at', 'last_running_check_at')

    def ext_job_id_link(self, obj):
        if obj and obj.ext_job_id:
            return get_job_id_link(obj.ext_job_id)


class CommandInline(ROTablularInline):
    model = models.Command
    fields = (
        'command_id',
        'minion_link',
        'status',
        'shipment_command',
        'created_at',
        'last_dispatch_attempt_at',
        'jobs_table',
    )
    raw_id_fields = ('minion',)

    def minion_link(self, obj):
        if obj and obj.minion_id:
            return obj.minion.link

    minion_link.short_description = 'Minion'

    def jobs_table(self, obj):
        if obj:
            return mark_safe(
                render_table(
                    ['Job Id', 'Ext Job Id', 'Status', 'Created at', 'Last Running Check at'],
                    [
                        [
                            row.job_id,
                            get_job_id_link(row.ext_job_id),
                            row.status,
                            row.created_at,
                            row.last_running_check_at,
                        ]
                        for row in obj.jobs.all()
                    ],
                )
            )


class ShipmentCommandInline(ROStackedInline):
    inlines = (CommandInline,)
    fields = ('shipment_link', 'shipment_command_link', 'type', 'arguments_pretty')
    model = models.ShipmentCommand

    def shipment_link(self, obj):
        if obj and obj.shipment:
            return obj.shipment.link

    shipment_link.short_description = 'Shipment'

    def shipment_command_link(self, obj):
        if obj:
            return obj.link

    shipment_command_link.short_description = 'Shipment Command'

    def arguments_pretty(self, obj):
        if obj and obj.arguments:
            return mark_safe('<br />'.join(obj.arguments))


class ShipmentAdmin(ROAdmin):
    inlines = (ShipmentCommandInline,)
    list_display = (
        'shipment_id',
        'hosts',
        'status',
        'total_count',
        'done_count',
        'errors_count',
        'timeout',
        'created_at',
    )
    fields = (
        'shipment_id_copy',
        'fqdns_link',
        'status',
        'parallel',
        'created_at',
        'updated_at',
        'timeout',
        'tracing',
        'jaeger_link',
        'stats',
    )
    list_filter = ('status',)
    search_fields = ('shipment_id', 'fqdns')

    def shipment_id_copy(self, obj):
        return with_copy(obj)

    shipment_id_copy.short_description = 'Shipment ID'

    def stats(self, obj):
        if obj:
            render = [
                ("Total", obj.total_count),
                ("Done", obj.done_count),
                ("Errors", obj.errors_count),
                ("Stop on Error", obj.stop_on_error_count),
                ("Other", obj.other_count),
            ]
            return mark_safe(
                "<table>" + "".join(["<tr><th>%s</th><td>%s</td></tr>" % (k, v) for k, v in render]) + "</table>"
            )

    def hosts(self, obj):
        if obj:
            if len(obj.fqdns) == 1:
                link_label = obj.fqdns[0]
            else:
                link_label = '%d Hosts' % len(obj.fqdns)
            return link_label

    def fqdns_link(self, obj):
        if obj:
            fqdn_dict = {m.fqdn: m.minion_id for m in models.Minion.objects.filter(fqdn__in=obj.fqdns)}
            return mark_safe(
                '<br />'.join([with_copy(fqdn, link('minion', fqdn_dict[fqdn], fqdn)) for fqdn in obj.fqdns])
            )

    fqdns_link.short_description = 'FQDNs'

    def jaeger_link(self, obj):
        if obj and obj.tracing:
            return mark_safe(make_jaeger_link(obj.tracing, settings.INSTALLATION))


class ShipmentCommandAdmin(ROAdmin):
    inlines = (CommandInline,)
    list_display = ('shipment_command_id', 'type', 'arguments')
    fields = ('shipment_command_id_copy', 'shipment_link', 'type', 'arguments', 'job_results_link')
    list_filter = ('type',)
    search_fields = ('shipment_id',)

    def shipment_command_id_copy(self, obj):
        return with_copy(obj.shipment_command_id)

    shipment_command_id_copy.short_description = 'Shipment Command ID'

    def shipment_link(self, obj):
        if obj and obj.shipment:
            return obj.shipment.link

    shipment_link.short_description = 'Shipment'

    def job_results_link(self, obj):
        if obj and obj.shipment_command_id:
            commands = obj.commands.all()
            jobs = models.Job.objects.filter(command__in=commands)
            ext_job_ids = [j.ext_job_id for j in jobs]
            jobs_results = models.JobResult.objects.filter(ext_job_id__in=ext_job_ids)
            return mark_safe(
                "<a href=/deploy/jobresult/?ext_job_id__in=%s>%d</a>"
                % (','.join([str(j) for j in ext_job_ids]), jobs_results.count())
            )

    job_results_link.short_description = 'Jobs'

    def change_view(self, request, object_id, form_url='', extra_context=None):
        self.get_params = request.GET
        return super().change_view(request, object_id, form_url, extra_context)

    def get_queryset(self, request):
        queryset = super().get_queryset(request)
        return queryset


class CommandAdmin(ROAdmin):
    inlines = (JobInline,)
    list_display = ('command_id', 'minion', 'status', 'shipment_command_link', 'created_at', 'last_dispatch_attempt_at')
    fields = ('command_id', 'minion_link', 'status', 'shipment_command_link', 'created_at', 'last_dispatch_attempt_at')
    raw_id_fields = ('minion', 'shipment_command')
    list_filter = ('status',)
    search_fields = ('minion__fqdn',)

    def minion_link(self, obj):
        if obj and obj.minion:
            return obj.minion.link

    minion_link.short_description = 'Minion'

    def shipment_command_link(self, obj):
        if obj and obj.shipment_command_id:
            return mark_safe(
                link('shipmentcommand', obj.shipment_command_id, obj.shipment_command_id)
                + "<br />"
                + obj.shipment_command.pretty
            )


class JobResultAdmin(ROAdmin):
    list_display = ('ext_job_id', 'fqdn', 'status', 'order_id', 'recorded_at')
    fields = (
        'ext_job_id_copy',
        'fqdn_copy',
        'job_result_id_copy',
        'status',
        'order_id',
        'recorded_at',
        'job',
        'result_filters',
        'result_pretty',
    )
    list_filter = ('status',)
    search_fields = ('ext_job_id', 'fqdn')
    readonly_fields = ('result_pretty', 'result_filters', 'job')
    exclude = ('result',)

    def ext_job_id_copy(self, obj):
        return with_copy(obj.ext_job_id)

    ext_job_id_copy.short_description = 'Ext Job ID'

    def fqdn_copy(self, obj):
        return with_copy(obj.fqdn)

    fqdn_copy.short_description = 'FQDN'

    def job_result_id_copy(self, obj):
        return with_copy(obj.job_result_id)

    job_result_id_copy.short_description = 'Job Result ID'

    def result_filters(self, obj):
        if obj:
            buttons = [
                ('result=true', 'result=true'),
                ('result=false', 'result=false'),
                ('result=false_parent', 'result=false only parent'),
                ('result=only_changed', 'only changed'),
                ('', 'all'),
            ]
            return mark_safe(
                "<br />".join(
                    [
                        '<a href="/deploy/jobresult/%s/change/?%s">%s</a>' % (obj.job_result_id, query, name)
                        for query, name in buttons
                    ]
                )
            )

    def result_pretty(self, obj):
        if obj:
            data = obj.result
            if data.get('fun') in {"state.highstate", "state.sls"}:
                data_return = []
                if type(data['return']) == list:
                    for row in data['return']:
                        data_return.append(row.split('\n'))
                    data_return.append('')
                    data['return'] = data_return
                    return mark_safe('<pre class="json">' + json.dumps(data) + '</pre>')
                if type(data['return']) != dict:
                    return mark_safe('<pre class="json">' + json.dumps(data) + '</pre>')
                for key, value in data['return'].items():
                    row = {}
                    row.update(value)
                    if row:
                        if 'result' in self.get_params:
                            if self.get_params['result'] == 'false_parent':
                                if row['result']:
                                    continue
                                if not row['result'] and 'One or more requisite failed' in row['comment']:
                                    continue

                            elif self.get_params['result'] == 'false' and row['result']:
                                continue
                            elif self.get_params['result'] == 'true' and not row['result']:
                                continue
                            elif (
                                self.get_params['result'] == 'only_changed'
                                and row['result']
                                and (row['changes'] == '{}' or not row['changes'])
                            ):
                                continue

                        row['state'] = key
                        if 'pchanges' in row and 'diff' in row['pchanges']:
                            row['pchanges']['diff'] = row['pchanges']['diff'].split('\n')
                        if 'comment' in row and '\n' in row['comment']:
                            row['comment'] = row['comment'].split('\n')

                        row['id'] = row.get('__id__')
                        if 'name' in row and 'state' in row:
                            state = row['state'].split('_|-')
                            if len(state) > 2:
                                row['id'] = '%s.%s %s' % (state[0], state[-1], row.get('name'))

                        data_return.append(row)
                data['return'] = sorted(data_return, key=lambda row: row['__run_num__'])
                if data['return']:
                    return mark_safe(render_job_result(data))
            return mark_safe('<pre class="json">' + json.dumps(data) + '</pre>')

    result_pretty.short_description = 'Result'

    def change_view(self, request, object_id, form_url='', extra_context=None):
        self.get_params = request.GET
        return super().change_view(request, object_id, form_url, extra_context)

    def job(self, obj):
        if obj:
            job = models.Job.objects.filter(ext_job_id=obj.ext_job_id).first()
            if not job:
                return
            command = job.command
            shipment_command = command.shipment_command
            shipment = shipment_command.shipment

            render = [
                ("Job ID", '%s (%s)' % (job.job_id, job.get_status_display())),
                (
                    "Command",
                    link('command', command.command_id, '%s (%s)' % (command.command_id, command.get_status_display())),
                ),
                (
                    "Shipment Command",
                    link('shipmentcommand', shipment_command.shipment_command_id, shipment_command.shipment_command_id),
                ),
                ("", shipment_command.pretty),
                (
                    "Shipment",
                    link(
                        'shipment',
                        shipment.shipment_id,
                        '%s (%s)' % (shipment.shipment_id, shipment.get_status_display()),
                    ),
                ),
                ("Created At", job.created_at),
                ("Last Running Check At", job.last_running_check_at),
                ("Running Checks Failed", job.running_checks_failed),
            ]
            return mark_safe(
                "<table>" + "".join(["<tr><th>%s</th><td>%s</td></tr>" % (k, v) for k, v in render]) + "</table>"
            )


admin.site.register(models.Group, GroupAdmin)
admin.site.register(models.Master, MasterAdmin)
admin.site.register(models.Minion, MinionAdmin)
admin.site.register(models.Shipment, ShipmentAdmin)
admin.site.register(models.ShipmentCommand, ShipmentCommandAdmin)
admin.site.register(models.Command, CommandAdmin)
admin.site.register(models.JobResult, JobResultAdmin)
