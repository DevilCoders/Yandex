from cloud.mdb.cleaner.internal.config import parse_config_file
from library.python import resource
from io import StringIO


def get_config() -> dict:
    return parse_config_file(StringIO(resource.find('config').decode('utf-8')))


def test_config():
    get_config()
