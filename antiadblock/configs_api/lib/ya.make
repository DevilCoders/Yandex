PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    utils.py
    jsonlogging.py
    internal_api.py
    db.py
    db_utils.py
    context.py
    app.py
    api_version.py
    models.py
    const.py

    validation/complex_validation.py
    validation/template.py
    validation/validation_utils.py

    service_api/api.py

    stat/charts_api.py

    metrics/metrics_client.py
    metrics/exceptions.py
    metrics/helpers.py
    metrics/metrics_api.py

    creation/creation.py

    auth/auth.py
    auth/auth_api.py
    auth/blackbox.py
    auth/idm_api.py
    auth/nodes.py
    auth/permission_models.py
    auth/permissions.py
    auth/webmaster.py

    audit/audit.py
    audit/audit_api.py

    bot/bot.py

    dashboard/dashboard_api.py
    dashboard/config.py
    dashboard/yql_queries.py

    heatmap/heatmap.py

    argus/argus_api.py
    argus/profiles.py
    argus/argus.py

    sonar/sonar.py

    infra/configs_infra_handler.py
    infra/infra_cache_manager.py
)

RESOURCE(
    keys/default.key        keys/default.key
    keys/default.pub        keys/default.pub
    keys/key.pub            keys/key.pub
    keys/test_tvm.key       keys/test_tvm.key
    creation/default.json   creation/default.json
    logging.ini             logging.ini
)

PEERDIR(
    antiadblock/libs/utils
    antiadblock/libs/infra
    contrib/python/cachetools
    library/python/yt
    contrib/python/PyJWT
    contrib/python/Flask
    contrib/python/Flask-SQLAlchemy
    contrib/python/sqlalchemy/sqlalchemy-1.2  # transition; see https://st.yandex-team.ru/CONTRIB-2042
    contrib/python/psycopg2
    contrib/python/gunicorn
    contrib/python/voluptuous
    contrib/python/Jinja2
    contrib/python/enum34
    contrib/python/pyre2
    contrib/python/requests
    library/python/tvmauth
    library/python/startrek_python_client
    library/python/charts_notes
    contrib/python/retry
    contrib/python/urllib3
    infra/yp_service_discovery/python/resolver
    infra/yp_service_discovery/api
    sandbox/common
    yt/python/client_with_rpc
)

END()
