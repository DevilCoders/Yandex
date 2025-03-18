PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/mock
    contrib/python/requests-mock
    library/python/startrek_python_client
)

TEST_SRCS(
    conftest.py
    common/url.py
    common/__init__.py
    common/startrek.py
    common/mock.py
    common/queues.py
    common/issues.py
    common/config.py
    common/bulkchange.py
    smoke/test_links.py
    smoke/test_issues_transition.py
    smoke/test_issues_general.py
    smoke/__init__.py
    smoke/test_queues.py
    smoke/test_bulkchange.py
    smoke/test_connection.py
    smoke/test_collections.py
    smoke/test_attachments.py
    smoke/test_issues_search.py
    smoke/test_comments.py
    smoke/test_versions.py
    smoke/conftest.py
)

END()

RECURSE(
    py2
    py3
)

