from os import environ
import pytest
from cloud.mdb.cleaner.internal.metadb import MetaDB
from cloud.mdb.recipes.postgresql.lib import env_postgres_addr
from .test_config import get_config


def test_visible_clusters_to_delete(metadb: MetaDB):
    with metadb:
        assert metadb.get_running_clusters_to_stop() == []


def test_get_stopped_clusters_to_delete(metadb: MetaDB):
    with metadb:
        assert metadb.get_stopped_clusters_to_delete() == []


def test_get_running_clusters_to_stop(metadb: MetaDB):
    with metadb:
        assert metadb.get_running_clusters_to_stop() == []


def test_is_readonly(metadb: MetaDB):
    with metadb:
        assert not metadb.is_read_only()


@pytest.fixture
def metadb() -> MetaDB:
    config = get_config()
    addr_env, port_env = env_postgres_addr('metadb')
    addr = environ.get(addr_env)
    port = environ.get(port_env)
    config['metadb'].update(
        dict(
            host=addr,
            port=port,
        )
    )
    return MetaDB(config)
