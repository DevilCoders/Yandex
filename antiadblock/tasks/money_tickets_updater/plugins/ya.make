PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    __init__.py
    awaps_morda.py
    bamboozled.py
    bypass_experiment.py
    experiments.py
    fraud.py
    run_argus.py
    statistics_by_partner_pageid.py
    unblock.py
    detect.py
    log_action.py
)

PEERDIR(
    library/python/tvmauth
    library/python/statface_client
    yql/library/python
    antiadblock/tasks/tools
)

END()
