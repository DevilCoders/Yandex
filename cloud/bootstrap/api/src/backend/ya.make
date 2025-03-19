PY3_LIBRARY(bootstrap.api)

OWNER(g:ycselfhost)

PEERDIR(
    library/cpp/resource
    library/python/deprecated/ticket_parser2

    library/python/blackbox
    library/python/tvm2

    contrib/python/flask-restplus
    contrib/python/gunicorn
    contrib/python/Jinja2
    contrib/python/psycopg2
    contrib/python/PyYAML
    contrib/python/schematics
    contrib/python/ujson
    contrib/python/Werkzeug
    contrib/python/Flask-Log-Request-ID

    cloud/bootstrap/common
)

IF (OS_SDK != "ubuntu-12" AND OS_LINUX)
    PEERDIR(
        contrib/python/cysystemd
    )
ENDIF()


RESOURCE(
    # swagger templates
    contrib/python/flask-restplus/flask_restplus/templates/swagger-ui.html /templates/swagger-ui.html
    contrib/python/flask-restplus/flask_restplus/templates/swagger-ui-css.html /templates/swagger-ui-css.html
    contrib/python/flask-restplus/flask_restplus/templates/swagger-ui-libs.html /templates/swagger-ui-libs.html

    # swagger static
    contrib/python/flask-restplus/flask_restplus/static/swagger-ui-bundle.js /static/swagger-ui-bundle.js
    contrib/python/flask-restplus/flask_restplus/static/swagger-ui-standalone-preset.js /static/swagger-ui-standalone-preset.js
    contrib/python/flask-restplus/flask_restplus/static/droid-sans.css /static/droid-sans.css
    contrib/python/flask-restplus/flask_restplus/static/swagger-ui.css /static/swagger-ui.css
    contrib/python/flask-restplus/flask_restplus/static/favicon-16x16.png /static/favicon-16x16.png
)

PY_SRCS(
    NAMESPACE bootstrap.api

    app.py
    logging.py

    # blueprints
    routes/__init__.py
    routes/helpers/aux_response.py
    routes/helpers/aux_resource.py
    routes/helpers/models.py
    routes/admin.py
    routes/hosts.py
    routes/instances.py
    routes/locks.py
    routes/salt_roles.py
    routes/svms.py
    routes/stands.py
    routes/host_configs_info.py

    # business logic
    core/auth.py
    core/constants.py
    core/decorators.py
    core/exceptions.py
    core/types.py
    core/utils.py
    core/locks.py
    core/instances.py
    core/instance_groups.py
    core/salt_roles.py
    core/stands.py
    core/cluster_maps.py
)

END()
