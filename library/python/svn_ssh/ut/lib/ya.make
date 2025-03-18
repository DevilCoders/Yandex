PY23_LIBRARY()

OWNER(g:tasklet)

PEERDIR(
    library/python/svn_ssh
)

TEST_SRCS(
    test_ssh.py
)

END()
