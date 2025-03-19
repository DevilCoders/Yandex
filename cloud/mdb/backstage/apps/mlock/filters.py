import cloud.mdb.backstage.lib.params as mod_params


LOCKS_PARAMS = [
    mod_params.RegexParam(
        key='lock_ext_id',
        is_opened=True,
    ),
    mod_params.RegexParam(
        key='fqdn',
        name='FQDN',
        is_opened=True,
        is_focused=True,
    ),
    mod_params.RegexParam(
        key='holder',
    ),
    mod_params.RegexParam(
        key='reason',
    ),
]


def get_locks_filters(request):
    return mod_params.Filter(
        LOCKS_PARAMS,
        request,
        url='/ui/mlock/ajax/locks',
        href='/ui/mlock/locks',
        js_object='mlock_locks_filter_object',
    ).parse()
