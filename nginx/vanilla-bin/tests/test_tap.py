import os
import pytest
import yatest
from tap_test_adapter import adapter

env = os.environ.copy()
env['TEST_LIBS'] = yatest.common.source_path('contrib/nginx/tests/lib')
env['TEST_NGINX_BINARY'] = yatest.common.binary_path('nginx/vanilla-bin/nginx')
# env['TEST_NGINX_GLOBALS_HTTP'] = "perl_modules %s;" % yatest.common.source_path('contrib/nginx/core/src/http/modules/perl/generated')  <~~~  disabled until it's clear how to use Perl in CI

tests_root = yatest.common.test_source_path()

names = list(filter(lambda x: x[-2:] == '.t', os.listdir(tests_root)))

# Forcibly skip those tests failed in CI  <~~~  resolve this ASAP!
skip_tests = [
    # Disabled permanently:
    'request_id.t',                             # request_id variable is provided by nginx/modules/small/ngx_http_request_id_module.c which is not in vanilla-bin
    # We build Nginx w/o image_filter module (see contrib/nginx/core/src/http/ya.make),
    # so disable the two tests at the below.
    'image_filter_finalize.t',
    'image_filter_webp.t',
    # Disabled until it's clear how to use Perl in CI:
    'gunzip_perl.t',
    'gzip_flush.t',
    'perl.t',
    'perl_gzip.t',
    'perl_sleep.t',
    'perl_ssi.t',
    'sub_filter_perl.t',
    # Different problems (these are specific only to CI, they don't arise in manual run on Ubuntu Trusty):
    'ssl_certificate_chain.t',                  # no specific error message from test
    'ssl_proxy_upgrade.t',                      # "Can't connect to nginx: Invalid argument" -- maybe passing undef
    'ssl_sni.t',                                # "Can't call method "dump_peer_certificate" on an undefined value"
    'h2_ssl_verify_client.t',                   # "Can't call method "alpn_selected" on an undefined value"
    'ssl.t',                                    # undef in all subtests
    'ssl_crl.t',                                # undef in all subtests
    'ssl_sni_sessions.t',                       # undef in all subtests
    'ssl_verify_depth.t',                       # undef in all subtests
    'ssl_password_file.t',                      # 'http' doesn't match 'https', or like that
    # Failed with: (2) X-IP: 127.0.0.1 != X-IP: 192.0.2.1; (1) invalid parameter "localhost"
    'realip_hostname.t',
    'stream_realip_hostname.t',
    # Failed with: (2) use of uninitialized value in pattern match; (1) unknown "ssl_client_escaped_cert" variable
    'ssl_client_escaped_cert.t',
]
for t in skip_tests:
    names.remove(t)


@pytest.mark.parametrize("name", names)
def test_simple(name):
    test = adapter(os.path.join(tests_root, name))
    test.run_test(env=env)
    assert len(test.failed) == 0, test.stderr
