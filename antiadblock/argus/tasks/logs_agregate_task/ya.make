PY3_PROGRAM(logs_agregate_task)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
    verdict_task.py
)

PEERDIR(
    contrib/python/Jinja2
    contrib/python/pandas
    contrib/python/numpy

    library/python/resource
    yql/library/python

    antiadblock/tasks/tools
    antiadblock/argus/bin/utils
    antiadblock/libs/elastic_search_client
)

RESOURCE(
    yql_tmpl/balancer.sql balancer
    yql_tmpl/cryprox.sql cryprox
    yql_tmpl/nginx.sql nginx
    yql_tmpl/bs_dsp_log.sql bs_dsp_log
    yql_tmpl/bs_event_log.sql bs_event_log
    yql_tmpl/bs_hit_log.sql bs_hit_log
)

END()
