PY3_PROGRAM(antirobot_gencfg)

OWNER(g:antirobot)

PY_SRCS(
    NAMESPACE antirobot
    gencfg.py
)

PY_MAIN(antirobot.gencfg)

PEERDIR(
    infra/yp_service_discovery/python/resolver
    infra/yp_service_discovery/api
)

END()
