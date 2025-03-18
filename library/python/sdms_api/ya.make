PY23_LIBRARY()

OWNER(qqq)

LICENSE(YandexOpen)

# copied from https://git.qe-infra.yandex-team.ru/projects/SEARCH_INFRA/repos/searchdms/browse/client/sdms_api/api.py
PY_SRCS(
    NAMESPACE sdms_api
    __init__.py
    sdms_api.py
)

PEERDIR(
    contrib/python/requests
)

END()
