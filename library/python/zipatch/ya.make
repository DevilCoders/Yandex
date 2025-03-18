PY23_LIBRARY()

OWNER(
    g:yatool
    workfork
)

PY_SRCS(
    __init__.py
    apply.py
    apply_svn.py
    const.py
    misc.py
    validate.py
    write.py
)

PEERDIR(
    contrib/python/patched/subvertpy
    library/python/path
    library/python/svn_ssh
    vcs/svn/wc/client
)

END()

RECURSE_FOR_TESTS(
    tests
)
