PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3
    contrib/python/six
    contrib/python/PyYAML
    contrib/python/ruamel.yaml
    contrib/python/toml

    library/python/django

    metrika/pylib/utils
    metrika/pylib/http
)

PY_SRCS(
    app.py
    ajax.py
    ping.py
    urls.py
    views.py
    models.py
    profile.py
    filters.py
    routers.py
    middleware.py
    context_processors.py
    templatetags/main/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/main/templates/main/base.html

    cloud/mdb/backstage/apps/main/templates/main/views/dashboard.html
    cloud/mdb/backstage/apps/main/templates/main/views/user_profile.html
    cloud/mdb/backstage/apps/main/templates/main/views/500.html
    cloud/mdb/backstage/apps/main/templates/main/views/404.html
    cloud/mdb/backstage/apps/main/templates/main/views/tools/vector_config.html
    cloud/mdb/backstage/apps/main/templates/main/views/tools/health_ua.html

    cloud/mdb/backstage/apps/main/templates/main/ajax/user_profile.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/failed_tasks.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/duties.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/search.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/stats_versions.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/stats_maintenance_tasks.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/tools/vector_schema.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/tools/health_ua.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/related_links.html
    cloud/mdb/backstage/apps/main/templates/main/ajax/audit.html

    cloud/mdb/backstage/apps/main/templates/main/includes/yandex_metrika_counter.html
    cloud/mdb/backstage/apps/main/templates/main/includes/tools/health_ua_by_availability.html
    cloud/mdb/backstage/apps/main/templates/main/includes/tools/health_ua_by_health.html
    cloud/mdb/backstage/apps/main/templates/main/includes/tools/health_ua_by_warning_geo.html
    cloud/mdb/backstage/apps/main/templates/main/includes/tools/health_ua_examples_link.html
)

NO_CHECK_IMPORTS()

END()
