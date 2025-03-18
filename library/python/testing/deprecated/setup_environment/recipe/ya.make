OWNER(g:yatest)

PY3_PROGRAM(setup_bin_dir)

PY_SRCS(__main__.py)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/deprecated/setup_environment
)

END()
