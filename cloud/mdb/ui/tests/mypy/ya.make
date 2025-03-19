PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    # This target is mandatory, it does all the job
    library/python/testing/types_test/py3
    # These targets are needed to include all the checked files and their
    # dependencies to test binary
    cloud/mdb/ui/internal
)

TEST_SRCS(conftest.py)

RESOURCE_FILES(cloud/mdb/ui/internal/mypy.ini)

ENV(DJANGO_SETTINGS_MODULE=cloud.mdb.ui.internal.mdbui.settings_test)

ENV(UI_LOAD_CONFIG_FROM_PKG=cloud.mdb.ui.internal,config.ini)

# Since mypy is rather slow it could be a good idea to force test
# to have MEDIUM or even LARGE size, and increase timeout.
SIZE(MEDIUM)

TIMEOUT(600)

END()
