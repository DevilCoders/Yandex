PY3_LIBRARY()

OWNER(g:billing-bcl)

PEERDIR(
    contrib/python/click
    contrib/python/pyasn1
    contrib/python/pyasn1-modules
    contrib/python/requests
)

PY_SRCS(
    TOP_LEVEL
    dssclient/__init__.py
    dssclient/auth.py
    dssclient/cli.py
    dssclient/client.py
    dssclient/endpoints/__init__.py
    dssclient/endpoints/base.py
    dssclient/endpoints/certificates.py
    dssclient/endpoints/documents.py
    dssclient/endpoints/policies.py
    dssclient/exceptions.py
    dssclient/http.py
    dssclient/resources/__init__.py
    dssclient/resources/base.py
    dssclient/resources/certificate.py
    dssclient/resources/document.py
    dssclient/resources/policy.py
    dssclient/settings.py
    dssclient/signing_params.py
    dssclient/stamping.py
    dssclient/utils.py
)

END()

RECURSE_FOR_TESTS(tests)
