PY3_LIBRARY()

OWNER(g:antirobot)

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/cbb/models
    antirobot/cbb/cbb_django/cbb/views
    antirobot/cbb/cbb_django/cbb/management
    antirobot/cbb/cbb_django/cbb/middlewares
    antirobot/cbb/cbb_django/cbb/templatetags
)

PY_SRCS(
    __init__.py
    apps.py
    urls.py
)

RESOURCE_FILES(PREFIX antirobot/cbb/cbb_django/cbb/
    # шаблоны
    templates/cbb/mail.html
    templates/cbb/404.html
    templates/cbb/nodb.html
    templates/cbb/blocks_list/active.html
    templates/cbb/blocks_list/history.html
    templates/cbb/index.html
    templates/cbb/group/add_service.html
    templates/cbb/group/blocks_list.html
    templates/cbb/group/edit_group.html
    templates/cbb/group/groups_table.html
    templates/cbb/group/groups.html
    templates/cbb/range/base_range.html
    templates/cbb/range/add_range.html
    templates/cbb/range/edit_range.html
    templates/cbb/range/view_range.html
    templates/cbb/403.html
    templates/cbb/500.html
)

END()

