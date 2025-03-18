PY2TEST()

OWNER(dmitko)

PEERDIR(
    library/python/cgroups/tests
)

SIZE(LARGE)

REQUIREMENTS(container:548368827)

# not for distbuild test
TAG(
    ya:fat
    ya:privileged
    ya:force_sandbox
)

NO_LINT()

END()
