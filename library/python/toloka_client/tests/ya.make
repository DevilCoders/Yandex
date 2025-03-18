OWNER(alexdrydew g:toloka-crowd-instruments)

PY3TEST()
FORK_TEST_FILES()
SIZE(MEDIUM)
PY_SRCS(_import_override.py)
TEST_SRCS(
    #test_ditamaps.py
    test_markdowns.py
    test_stubs.py
)
DATA(
    arcadia/library/python/toloka-kit
    arcadia/library/python/crowd-kit
)
PEERDIR(
    library/python/toloka_client/src/tk_stubgen
    library/python/toloka-kit
    library/python/crowd-kit
)
REQUIREMENTS(ram:13)

END()
