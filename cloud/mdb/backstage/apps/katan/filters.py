import cloud.mdb.backstage.lib.params as mod_params


CLUSTERS_PARAMS = [
    mod_params.QuerysetListParam(
        key='cluster_id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
]

HOSTS_PARAMS = [
    mod_params.QuerysetListParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetListParam(
        key='cluster_id',
        name='Cluster ID',
        queryset_key='cluster__cluster_id__in',
        is_opened=True,
        is_focused=True,
    ),
]

ROLLOUTS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='rollout_id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
]

SCHEDULES_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='schedule_id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
]


def get_clusters_filters(request):
    return mod_params.Filter(
        CLUSTERS_PARAMS,
        request,
        url='/ui/katan/ajax/clusters',
        href='/ui/katan/clusters',
        js_object='katan_clusters_filter_object',
    ).parse()


def get_hosts_filters(request):
    return mod_params.Filter(
        HOSTS_PARAMS,
        request,
        url='/ui/katan/ajax/hosts',
        href='/ui/katan/hosts',
        js_object='katan_hosts_filter_object',
    ).parse()


def get_rollouts_filters(request):
    return mod_params.Filter(
        ROLLOUTS_PARAMS,
        request,
        url='/ui/katan/ajax/rollouts',
        href='/ui/katan/rollouts',
        js_object='katan_rollouts_filter_object',
    ).parse()


def get_schedules_filters(request):
    return mod_params.Filter(
        SCHEDULES_PARAMS,
        request,
        url='/ui/katan/ajax/schedules',
        href='/ui/katan/schedules',
        js_object='katan_schedules_filter_object',
    ).parse()
