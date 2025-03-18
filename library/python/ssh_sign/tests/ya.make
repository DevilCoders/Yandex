OWNER(
    pg
    g:yatool
)

PY23_LIBRARY()

TEST_SRCS(test_ssh_sign_load_key.py)

PEERDIR(
    library/python/ssh_sign
    contrib/python/paramiko
)

END()

RECURSE(
    py2
    py3
)