PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    check_activity.py
    fix_block_descr.py
    generate_queries.py
    check_db.py
    fix_xtra_large.py
    grant_role.py
    check_history_limbos.py
    flush_expired_blocks.py
    __init__.py
    ddl.py
    flush_limbos.py
    remove_old_history.py
    emptify_group.py
)

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/cbb/library
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
)

END()
