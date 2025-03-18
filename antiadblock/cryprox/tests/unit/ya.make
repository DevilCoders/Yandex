PY2TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    __init__.py
    conftest.py
    test_body_crypt.py
    test_body_replace.py
    test_cookie_secret_key.py
    test_config_utils.py
    test_crypt_methods.py
    test_crypt_url.py
    test_crypturlprefix.py
    test_duration_decorator.py
    test_csp_second_domain.py
    test_generate_seed.py
    test_get_active_experiments.py
    test_js_crypt.py
    test_jsonify_meta.py
    test_ip_utils.py
    test_remove_headers.py
    test_resolver.py
    test_rule_evade_step.py
    test_tools.py
    test_trailing_slash.py
    test_validate_configs.py
    test_user_agent_parsing.py
)

PEERDIR(
    contrib/python/beautifulsoup4
    contrib/python/PyHamcrest
    contrib/python/mock

    antiadblock/cryprox/cryprox
    antiadblock/cryprox/tests/lib
    antiadblock/libs/decrypt_url
)

PY_DOCTESTS(
    antiadblock.cryprox.cryprox.service.metrics
    antiadblock.cryprox.cryprox.common.cryptobody
    antiadblock.cryprox.cryprox.common.tools.misc
)

END()
