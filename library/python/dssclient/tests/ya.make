OWNER(g:billing-bcl)

PY3TEST()

PEERDIR(
    library/python/dssclient
    contrib/python/pytest-responsemock
)

TEST_SRCS(
    conftest.py
    test_auth.py
    test_certificates.py
    test_documents.py
    test_policies.py
    test_signing_params.py
    test_stamping.py
)

RESOURCE_FILES(PREFIX library/python/dssclient/tests/
    fixtures/cert
    fixtures/certificate_requests.json
    fixtures/certificates.json
    fixtures/signatures_ok.json
    fixtures/signing_policy.json
    fixtures/users_policy.json
)

END()
