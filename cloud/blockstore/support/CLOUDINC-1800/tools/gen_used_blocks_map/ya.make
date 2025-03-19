PY3_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
    ext4.py
    ntfs.py
    xfs.py
)

PEERDIR(
)

END()

RECURSE_FOR_TESTS(tests)
