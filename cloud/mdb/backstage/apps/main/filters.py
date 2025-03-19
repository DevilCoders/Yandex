import cloud.mdb.backstage.lib.params as mod_params


STATS_VERSIONS_PARAMS = [
    mod_params.RegexParam(
        key='component',
        is_focused=True,
        is_opened=True,
    ),
    mod_params.RegexParam(
        key='major_version',
    ),
    mod_params.RegexParam(
        key='minor_version',
    ),
    mod_params.RegexParam(
        key='edition',
    ),
]


STATS_MAINTENANCE_TASKS_PARAMS = [
    mod_params.RegexParam(
        key='component',
        is_focused=True,
        is_opened=True,
    ),
    mod_params.RegexParam(
        key='env',
    ),
    mod_params.RegexParam(
        key='status',
    ),
    mod_params.RegexParam(
        key='config_id',
    ),
]


AUDIT_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='obj_pk',
        name='Object',
        is_focused=True,
        is_opened=True,
    ),
    mod_params.QuerysetRegexParam(
        key='message',
        is_opened=True,
    ),
    mod_params.QuerysetRegexParam(
        key='user_comment',
        name='Comment',
        is_opened=True,
    ),
    mod_params.QuerysetRegexParam(
        key='obj_app',
        name='App'
    ),
    mod_params.QuerysetRegexParam(
        key='obj_model',
        name='Model'
    ),
    mod_params.QuerysetRegexParam(
        key='obj_action',
        name='Action'
    ),
    mod_params.QuerysetRegexParam(
        key='username',
    ),
]


def get_stats_versions_filters(request):
    return mod_params.Filter(
        STATS_VERSIONS_PARAMS,
        request,
        url='/ui/main/ajax/stats/versions',
        href='/ui/main/stats/versions',
        js_object='main_stats_versions_filter_object',
    ).parse()


def get_stats_maintenance_tasks_filters(request):
    return mod_params.Filter(
        STATS_MAINTENANCE_TASKS_PARAMS,
        request,
        url='/ui/main/ajax/stats/maintenance_tasks',
        href='/ui/main/stats/maintenance_tasks',
        js_object='main_stats_maintenance_tasks_filter_object',
    ).parse()


def get_audit_filters(request):
    return mod_params.Filter(
        AUDIT_PARAMS,
        request,
        url='/ui/main/ajax/audit',
        href='/ui/main/audit',
        js_object='audit_filter_object',
    ).parse()
