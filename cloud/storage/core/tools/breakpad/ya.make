PY2_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
    contrib/python/requests
    library/python/retry
)

PY_SRCS(
    command.py
    conductor.py
    core_checker.py
    coredump.py
    coredump_formatter.py
    crash_info.py
    crash_processor.py
    error_collector.py
    launcher.py
    oom_checker.py
    sender.py
)

END()

RECURSE(
    launcher
    sender
)
