PY2_PROGRAM()

OWNER(g:yatool dmitko)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/qemu_kvm/lib
)

PY_SRCS(__main__.py)

END()

RECURSE_FOR_TESTS(
    example
    example-pytest
    example-gtest
    test
)
