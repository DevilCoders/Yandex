OWNER(g:cloud-nbs)

PY23_LIBRARY()

PY_SRCS(__init__.py)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/qemu
    contrib/python/retrying
    devtools/ya/core/config
    kikimr/ci/libraries
    library/python/fs
    library/python/testing/recipe
)

END()
