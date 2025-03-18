PY2TEST()

OWNER(
    dmtrmonakhov
)

TEST_SRCS(
    test_guest_cli.py
    test_dry_run.py
)

PEERDIR(
    contrib/python/python-slugify
)

DEPENDS(
     tools/sandboxctl/bin
)

TAG(ya:external)

REQUIREMENTS(network:full)

FORK_SUBTESTS()
SIZE(MEDIUM)
END()
