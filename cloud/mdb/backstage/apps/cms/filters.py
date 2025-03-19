import cloud.mdb.backstage.lib.params as mod_params

import cloud.mdb.backstage.apps.cms.models as mod_models


DECISIONS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='fqdn',
        name='FQDN',
        queryset_key='request__fqdns__regex',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='duty_routine',
        choices=mod_models.DecisionDutyRoutine.all,
        is_opened=True,
    ),
    mod_params.QuerysetRegexParam(
        key='action',
        queryset_key='request__name__regex',
    ),
    mod_params.QuerysetRegexParam(
        key='mutations_log',
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='status',
        choices=mod_models.DecisionStatus.all,
    ),
]


INSTANCE_OPERATIONS_PARAMS = [
    mod_params.QuerysetUuidListParam(
        key='operation_id',
        name='ID',
    ),
    mod_params.QuerysetRegexParam(
        key='author',
    ),
    mod_params.QuerysetRegexParam(
        key='instance_id',
    ),
    mod_params.QuerysetRegexParam(
        key='comment',
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='status',
        choices=mod_models.InstanceOperationStatus.all,
        css_class_prefix='backstage-cms-instance-operation-status',
    ),
    mod_params.QuerysetMultipleChoicesParam(
        key='operation_type',
        name='Type',
        choices=mod_models.InstanceOperationType.all,
        css_class_prefix='backstage-cms-instance-operation-type'
    ),
]


def get_decisions_filters(request):
    return mod_params.Filter(
        DECISIONS_PARAMS,
        request,
        url='/ui/cms/ajax/decisions',
        href='/ui/cms/decisions',
        js_object='cms_decisions_filter_object',
    ).parse()


def get_instance_operations_filters(request):
    return mod_params.Filter(
        INSTANCE_OPERATIONS_PARAMS,
        request,
        url='/ui/cms/ajax/instance_operations',
        href='/ui/cms/instance_operations',
        js_object='cms_instance_operations_filter_object',
    ).parse()
