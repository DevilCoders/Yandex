PY3_LIBRARY()

OWNER(g:ycselfhost)

PY_SRCS(
    NAMESPACE bootstrap.common

    exceptions.py
    config.py
    console_ui.py
    utils.py

    argparse/types.py

    rdbms/decorators.py
    rdbms/exceptions.py
    rdbms/db.py
)

END()
