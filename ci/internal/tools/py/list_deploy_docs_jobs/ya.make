PY2_PROGRAM()
NO_CHECK_IMPORTS()

OWNER(g:ci)

PY_MAIN(ci.internal.tools.py.list_deploy_docs_jobs.main)

PY_SRCS(
    main.py
)

PEERDIR(
    testenv/jobs/docs

    testenv/core/engine
)

END()
