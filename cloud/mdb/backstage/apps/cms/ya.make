PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3

    library/python/django
)

PY_SRCS(
    app.py
    urls.py
    models.py
    filters.py
    views/decisions.py
    views/instance_operations.py
    actions/decisions.py
    actions/instance_operations.py
    templatetags/cms/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/cms/templates/cms/instance_operations/instance_operations.html
    cloud/mdb/backstage/apps/cms/templates/cms/instance_operations/action_dialog.html
    cloud/mdb/backstage/apps/cms/templates/cms/instance_operations/sections/common.html

    cloud/mdb/backstage/apps/cms/templates/cms/decisions/decisions.html
    cloud/mdb/backstage/apps/cms/templates/cms/decisions/action_dialog.html
    cloud/mdb/backstage/apps/cms/templates/cms/decisions/sections/common.html

    cloud/mdb/backstage/apps/cms/templates/cms/templatetags/pretty_log.html
    cloud/mdb/backstage/apps/cms/templates/cms/templatetags/decision_fqdns.html
    cloud/mdb/backstage/apps/cms/templates/cms/templatetags/decision_action.html
    cloud/mdb/backstage/apps/cms/templates/cms/templatetags/decision_author.html
)

END()
