PY2_LIBRARY()

OWNER(
    g:balancer
)

NO_NEED_CHECK()

PEERDIR(
    infra/awacs/vendor/awacs
    infra/nanny/sepelib/yandex

    gencfg/custom_generators/balancer_gencfg/configs/addrs
    gencfg/custom_generators/balancer_gencfg/configs/any
    gencfg/custom_generators/balancer_gencfg/configs/improxy
    gencfg/custom_generators/balancer_gencfg/configs/gencfg
    gencfg/custom_generators/balancer_gencfg/configs/l7heavy
    gencfg/custom_generators/balancer_gencfg/configs/load_testing
)

PY_SRCS(
    TOP_LEVEL
    src/constants.py
    src/constraint_checker.py
    src/generator.py
    src/groupmap.py
    src/lua_globals.py
    src/lua_processor.py
    src/lua_printer.py
    src/macroses.py
    src/macro_processor.py
    src/mapping.py
    src/modules.py
    src/module_processor.py
    src/optionschecker.py
    src/optimizer.py
    src/parsecontext.py
    src/retry.py
    src/transports/base_db_transport.py
    src/transports/gencfg_api_transport.py
    src/transports/__init__.py
    src/trusted_nets.py
    src/utils.py
    balancer_config.py
)

END()
