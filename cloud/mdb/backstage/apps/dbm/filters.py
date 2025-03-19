import cloud.mdb.backstage.lib.params as mod_params


CLUSTERS_PARAMS = [
    mod_params.QuerysetListParam(
        key='name',
        is_opened=True,
        is_focused=True,
    ),
]

CONTAINERS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='dom0_fqdn',
        name='Dom0 FQDN ',
        queryset_key='dom0host__fqdn__regex',
    ),
    mod_params.QuerysetRegexParam(
        key='cluster_name',
        name='Cluster name ',
        queryset_key='cluster__name__regex',
    ),
]

DOM0_HOSTS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='project',
        name='Project name',
        queryset_key='project__name__regex',
    ),
    mod_params.QuerysetRegexParam(
        key='geo',
        name='Geo',
        queryset_key='location__geo__regex',
    ),
    mod_params.QuerysetIntListParam(
        key='generation',
        name='Generation',
        queryset_key='generation__in',
    ),
]

PROJECTS_PARAMS = [
    mod_params.QuerysetRegexParam(
        key='name',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.QuerysetRegexParam(
        key='description',
    ),
]

TRANSFERS_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='id',
        name='ID',
        is_opened=True,
        is_focused=True,
    ),
]

RESERVED_RESOURCES_PARAMS = [
    mod_params.QuerysetIntListParam(
        key='generation',
        queryset_key='generation__in',
        is_opened=True,
        is_focused=True,
    ),
]


def get_clusters_filters(request):
    return mod_params.Filter(
        CLUSTERS_PARAMS,
        request,
        url='/ui/dbm/ajax/clusters',
        href='/ui/dbm/clusters',
        js_object='dbm_clusters_filter_object',
    ).parse()


def get_containers_filters(request):
    return mod_params.Filter(
        CONTAINERS_PARAMS,
        request,
        url='/ui/dbm/ajax/containers',
        href='/ui/dbm/containers',
        js_object='dbm_containers_filter_object',
    ).parse()


def get_dom0_hosts_filters(request):
    return mod_params.Filter(
        DOM0_HOSTS_PARAMS,
        request,
        url='/ui/dbm/ajax/dom0_hosts',
        href='/ui/dbm/dom0_hosts',
        js_object='dbm_dom0_hosts_filter_object',
    ).parse()


def get_projects_filters(request):
    return mod_params.Filter(
        PROJECTS_PARAMS,
        request,
        url='/ui/dbm/ajax/projects',
        href='/ui/dbm/projects',
        js_object='dbm_projects_filter_object',
    ).parse()


def get_reserved_resources_filters(request):
    return mod_params.Filter(
        RESERVED_RESOURCES_PARAMS,
        request,
        url='/ui/dbm/ajax/reserved_resources',
        href='/ui/dbm/reserved_resources',
        js_object='dbm_reserved_resources_filter_object',
    ).parse()


def get_transfers_filters(request):
    return mod_params.Filter(
        TRANSFERS_PARAMS,
        request,
        url='/ui/dbm/ajax/transfers',
        href='/ui/dbm/transfers',
        js_object='dbm_transfers_filter_object',
    ).parse()
