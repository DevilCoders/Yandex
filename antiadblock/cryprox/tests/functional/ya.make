PY2TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    __init__.py
    conftest.py
    test_accel_redirect.py
    test_adb_enabled.py
    test_appcry_handler.py
    test_basic.py
    test_bypass_by_uids.py
    test_body_replace.py
    test_response_cache.py
    test_cached_detect.py
    test_chkaccess.py
    test_classify_url.py
    test_client_side_replace.py
    test_config_bk_adfox.py
    test_config_reload.py
    test_cookie_handler.py
    test_cookies.py
    test_counters_test_tag.py
    test_crawler.py
    test_crypt_content_api.py
    test_crypt_content_types.py
    test_crypt_cookie.py
    test_crypt_methods.py
    test_crypt_url_handler.py
    test_crypted_host.py
    test_decrypt_methods.py
    test_experiment.py
    test_extuid.py
    test_fetch_url_network_fail.py
    test_fetch_url_service_slb.py
    test_fixup_url.py
    test_generate_cookie_handler.py
    test_headers.py
    test_hierarchical_configs.py
    test_hostname_mapping.py
    test_http_methods.py
    test_inject_inline_js.py
    test_internal_experiments.py
    test_js_crypt.py
    test_meta_hidden_args_header.py
    test_metrics.py
    test_new_detect_script.py
    test_proxy_pcode_loader.py
    test_redirect.py
    test_request_id.py
    test_resource_protect.py
    test_rtb_auction_response_processing.py
    test_service_handlers.py
    test_stub_server.py
    test_x_forwarded_proto.py
    test_get_yandex_nets.py
)

PEERDIR(
    contrib/python/PyHamcrest
    contrib/python/requests
    contrib/python/pytest
    contrib/python/beautifulsoup4
    contrib/python/mock

    antiadblock/cryprox/cryprox
    antiadblock/cryprox/tests/lib
    antiadblock/libs/decrypt_url
    antiadblock/libs/tornado_redis
)

DEPENDS(
    antiadblock/cryprox/cryprox_run
)

DATA(
    arcadia/antiadblock/cryprox/tests/functional/stubs
    arcadia/antiadblock/encrypter/tests/test_keys.txt
)

RESOURCE(
    resources/test_rtb_auction_response_processing/css_decoded.css resources/test_rtb_auction_response_processing/css_decoded.css
    resources/test_rtb_auction_response_processing/html_decoded.html resources/test_rtb_auction_response_processing/html_decoded.html
    resources/test_rtb_auction_response_processing/adfox_banner_html.html resources/test_rtb_auction_response_processing/adfox_banner_html.html
    resources/test_rtb_auction_response_processing/bk_rtb_auction_ssr_response.json resources/test_rtb_auction_response_processing/bk_rtb_auction_ssr_response.json
    resources/test_rtb_auction_response_processing/bk_rtb_auction_widget_ssr_response.json resources/test_rtb_auction_response_processing/bk_rtb_auction_widget_ssr_response.json
    resources/test_rtb_auction_response_processing/bk_rtb_auction_response.json resources/test_rtb_auction_response_processing/bk_rtb_auction_response.json
    resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json
    resources/test_rtb_auction_response_processing/adfox_rtb_getcode_auction_response.js resources/test_rtb_auction_response_processing/adfox_rtb_getcode_auction_response.js
    resources/test_rtb_auction_response_processing/bk_without_html_rtb_auction_response.js resources/test_rtb_auction_response_processing/bk_without_html_rtb_auction_response.js
    resources/test_rtb_auction_response_processing/bk_without_rtb_auction_response.json resources/test_rtb_auction_response_processing/bk_without_rtb_auction_response.json
    resources/test_rtb_auction_response_processing/crypted_rambler_banner_html.html resources/test_rtb_auction_response_processing/crypted_rambler_banner_html.html
    resources/test_rtb_auction_response_processing/rambler_auction_response_client_side.jsp resources/test_rtb_auction_response_processing/rambler_auction_response_client_side.jsp
    resources/test_rtb_auction_response_processing/rambler_auction_response_server_side.jsp resources/test_rtb_auction_response_processing/rambler_auction_response_server_side.jsp
    resources/test_rtb_auction_response_processing/smartbanner_data.json resources/test_rtb_auction_response_processing/smartbanner_data.json
    resources/test_rtb_auction_response_processing/vast_decoded.xml resources/test_rtb_auction_response_processing/vast_decoded.xml
    resources/test_rtb_auction_response_processing/vast_decoded_crypted.xml resources/test_rtb_auction_response_processing/vast_decoded_crypted.xml
    resources/test_rtb_auction_response_processing/bk_rtb_auction_response_external_base_path.json resources/test_rtb_auction_response_processing/bk_rtb_auction_response_external_base_path.json
    resources/test_rtb_auction_response_processing/bk_rtb_auction_response_external_base_path_expected_crypted.json resources/test_rtb_auction_response_processing/bk_rtb_auction_response_external_base_path_expected_crypted.json

    resources/test_js_crypt/html_with_js_original.html resources/test_js_crypt/html_with_js_original.html
    resources/test_js_crypt/expected_crypted.js resources/test_js_crypt/expected_crypted.js
    resources/test_js_crypt/json_code_original.json resources/test_js_crypt/json_code_original.json
    resources/test_js_crypt/expected_crypted_strings.json resources/test_js_crypt/expected_crypted_strings.json

    resources/test_crypt_non_base64_html_meta/non_base64_html.js resources/test_crypt_non_base64_html_meta/non_base64_html.js
    resources/test_crypt_non_base64_html_meta/non_base64_html_2.json resources/test_crypt_non_base64_html_meta/non_base64_html_2.json
)

REQUIREMENTS(network:full)

SIZE(MEDIUM)



END()
