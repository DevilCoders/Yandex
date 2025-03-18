OWNER(
    g:yatool
    workfork
)

PY23_TEST()

TEST_SRCS(
    common.py
    conftest.py
    test_apply.py
    test_apply_svn.py
    test_zipatch.py
)

PEERDIR(
    devtools/ya/exts
    library/python/func
    library/python/zipatch
    library/python/windows
    vcs/svn/wc/client
)

END()
