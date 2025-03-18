OWNER(g:yatool dmitko dmtrmonakhov)

PY2_LIBRARY()

PY_SRCS(__init__.py)

PEERDIR(
    devtools/ya/core/config
    library/python/fs
    contrib/python/retrying
    contrib/python/qemu
    contrib/python/PyYAML
    library/python/testing/recipe
)

END()
