PY3_PROGRAM(sandbox_importer_task)

OWNER(g:cloud-ps)

PEERDIR(
    cloud/ps/gore/scripts/sanbox_jobs/src
    cloud/ps/gore/scripts/sanbox_jobs/config
)

PY_SRCS(
    __main__.py
)

END()
