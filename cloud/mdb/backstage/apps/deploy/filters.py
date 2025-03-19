import cloud.mdb.backstage.lib.params as mod_params

import cloud.mdb.backstage.apps.deploy.models as mod_models


SHIPMENTS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='shipment_id',
        name='ID',
    ),
    mod_params.QuerysetListParam(
        key='fqdns',
        name='FQDN',
        queryset_key='fqdns__contains',
    ),
]

JOB_RESULTS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='job_result_id',
        name='ID',
    ),
    mod_params.QuerysetIntListParam(
        key='ext_job_id',
        name='Ext Job ID',
    ),
    mod_params.QuerysetRegexParam(
        key='fqdn',
        name='FQDN',
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='status',
        choices=mod_models.JobResultStatus.all,
    ),

]

COMMANDS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='command_id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='minion',
        queryset_key='minion__fqdn__regex',
        is_opened=True,
        is_focused=True,
    ),
]

GROUPS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='name',
        is_opened=True,
        is_focused=True,
    ),
]

MASTERS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetListParam(
        key='group',
        name='Group',
        queryset_key='group__name__in',
    ),
]

MINIONS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetListParam(
        key='group',
        name='Group',
        queryset_key='group__name__in',
    ),
    mod_params.QuerysetListParam(
        key='master',
        name='Master',
        queryset_key='master__fqdn__in',
    ),
]

SHIPMENT_COMMANDS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='shipment_command_id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='type',
        is_opened=True,
        is_focused=True,
    ),
]


def get_shipments_filters(request):
    return mod_params.Filter(
        SHIPMENTS_PARAMS,
        request,
        url='/ui/deploy/ajax/shipments',
        href='/ui/deploy/shipments',
        js_object='deploy_shipments_filter_object',
    ).parse()


def get_job_results_filters(request):
    return mod_params.Filter(
        JOB_RESULTS_PARAMS,
        request,
        url='/ui/deploy/ajax/job_results',
        href='/ui/deploy/job_results',
        js_object='deploy_job_results_filter_object',
    ).parse()


def get_commands_filters(request):
    return mod_params.Filter(
        COMMANDS_PARAMS,
        request,
        url='/ui/deploy/ajax/commands',
        href='/ui/deploy/commands',
        js_object='deploy_commands_filter_object',
    ).parse()


def get_groups_filters(request):
    return mod_params.Filter(
        GROUPS_PARAMS,
        request,
        url='/ui/deploy/ajax/groups',
        href='/ui/deploy/groups',
        js_object='deploy_groups_filter_object',
    ).parse()


def get_masters_filters(request):
    return mod_params.Filter(
        MASTERS_PARAMS,
        request,
        url='/ui/deploy/ajax/masters',
        href='/ui/deploy/masters',
        js_object='deploy_masters_filter_object',
    ).parse()


def get_minions_filters(request):
    return mod_params.Filter(
        MINIONS_PARAMS,
        request,
        url='/ui/deploy/ajax/minions',
        href='/ui/deploy/minions',
        js_object='deploy_minions_filter_object',
    ).parse()


def get_shipment_commands_filters(request):
    return mod_params.Filter(
        SHIPMENT_COMMANDS_PARAMS,
        request,
        url='/ui/deploy/ajax/shipment_commands',
        href='/ui/deploy/shipment_commands',
        js_object='deploy_shipment_commands_filter_object',
    ).parse()
