PY3TEST()

OWNER(g:nocdev-ops)

TEST_SRCS(
    test_import.py
    test_conductor.py
)

PEERDIR(
    admins/yandex_salt_components
)

END()
