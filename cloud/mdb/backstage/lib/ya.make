PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3
    contrib/python/six
    contrib/python/sqlparse
    contrib/python/Pygments
    contrib/python/PyYAML

    library/python/django
    library/python/json

    metrika/pylib/utils
    metrika/pylib/http
    metrika/pylib/structures/dotdict
)

PY_SRCS(
    log.py
    apps.py
    gore.py
    health.py
    search.py
    params.py
    helpers.py
    response.py
    middleware.py
    installation.py
    templatetags/lib/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/lib/templates/lib/object.html
    cloud/mdb/backstage/lib/templates/lib/objects_list.html
    cloud/mdb/backstage/lib/templates/lib/ajax/modal.html
    cloud/mdb/backstage/lib/templates/lib/ajax/objects_list.html
    cloud/mdb/backstage/lib/templates/lib/ajax/action_dialog.html
    cloud/mdb/backstage/lib/templates/lib/ajax/object.html
    cloud/mdb/backstage/lib/templates/lib/includes/filters_errors.html
    cloud/mdb/backstage/lib/templates/lib/includes/html_paginator.html
    cloud/mdb/backstage/lib/templates/lib/includes/js_paginator.html
    cloud/mdb/backstage/lib/templates/lib/includes/sidebar_footer.html
    cloud/mdb/backstage/lib/templates/lib/includes/sidebar_filters.html
    cloud/mdb/backstage/lib/templates/lib/includes/filters_no_objects.html
    cloud/mdb/backstage/lib/templates/lib/includes/js_tab_activator.html
    cloud/mdb/backstage/lib/templates/lib/includes/html_traceback.html
    cloud/mdb/backstage/lib/templates/lib/includes/html_json.html
    cloud/mdb/backstage/lib/templates/lib/templatetags/dt_formatted.html
    cloud/mdb/backstage/lib/templates/lib/templatetags/selectable_th_checkbox.html
    cloud/mdb/backstage/lib/templates/lib/templatetags/selectable_td_checkbox.html
    cloud/mdb/backstage/lib/templates/lib/templatetags/pillar_change.html
    cloud/mdb/backstage/lib/templates/lib/templatetags/action_input.html
    cloud/mdb/backstage/lib/templates/lib/templatetags/collapsible.html
)

NO_CHECK_IMPORTS()

END()
