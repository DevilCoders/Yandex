import pytest
from gaux.aux_utils import run_command

is_traverse_test = pytest.mark.is_traverse_test


@is_traverse_test
def test_traverse_wbe(wbe):
    _test_traverse(wbe)


@is_traverse_test
def test_traverse_api(api):
    _test_traverse(api)


def _test_traverse(instance):
    host = instance.api.get_host()
    port = instance.api.get_port()
    backend_type = instance.api.get_backend_type()
    db_type = instance.api.get_db_type()

    if db_type != "tags":
        raise Exception("Invalid db type \"%s\", should run on db type \"%s\"" % (db_type, "tags"))

    run_command(["web_utils/dolbilo.py",
                 "-p", str(port),
                 "--host", str(host),
                 "-b", backend_type,
                 "-d", "10",
                 "--tags-filter", "recent",
                 "--continue-on-error"])
