PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    __init__.py
    common/__init__.py
    common/config_utils.py
    common/cry.py
    common/cryptobody.py
    common/juggler_utils.py
    common/visibility_protection.py
    common/resource_utils.py

    common/tools/__init__.py
    common/tools/bypass_by_uids.py
    common/tools/crypt_cookie_marker.py
    common/tools/experiments.py
    common/tools/internal_experiments.py
    common/tools/jsonify_meta.py
    common/tools/js_minify.py
    common/tools/misc.py
    common/tools/ip_utils.py
    common/tools/regexp.py
    common/tools/tornado_retry_request.py
    common/tools/ua_detector.py
    common/tools/url.py
    common/tools/porto_metrics.py

    config/__init__.py
    config/ad_systems.py
    config/adfox.py
    config/bk.py
    config/config.py
    config/rambler.py
    config/service.py
    config/static_config.py
    config/system.py

    service/__init__.py
    service/body_encrypter.py
    service/service.py
    service/service_application.py
    service/worker_application.py
    service/resolver.py
    service/metrics.py
    service/jslogger.py
    service/decorators.py
    service/action.py
    service/pipeline.py

    url/transform/rule_evade_step.py
)

RESOURCE(
    common/js_func/decode_url_func.js /cryprox/common/js_func/decode_url_func.js
    common/js_func/encode_css_func.js /cryprox/common/js_func/encode_css_func.js
    common/js_func/encode_url_func.js /cryprox/common/js_func/encode_url_func.js
    common/js_func/is_encoded_url_func.js /cryprox/common/js_func/is_encoded_url_func.js

    common/test_tvm_ticket.key /cryprox/common/test_tvm_ticket.key

    url/transform/rule_evade_list url/transform/rule_evade_list

    metrika/uatraits/data/browser.xml  /browser.xml
)

PEERDIR(
    yabs/server/libs/py_filter_record

    contrib/python/cachetools

    contrib/python/tornado/tornado-4
    contrib/python/enum34
    library/python/tvmauth
    contrib/python/numpy
    contrib/python/netaddr
    contrib/python/pyre2
    contrib/python/cchardet
    library/python/resource
    contrib/python/pycrypto
    contrib/python/psutil
    contrib/python/tenacity
    contrib/python/monotonic
    metrika/uatraits/python
    metrika/uatraits/data
    antiadblock/encrypter
    antiadblock/libs/decrypt_url
    antiadblock/libs/tornado_redis
    infra/porto/api_py
)

END()
