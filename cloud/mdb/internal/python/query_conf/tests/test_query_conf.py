from cloud.mdb.internal.python.query_conf.tests.sample import QUERIES


def test_load_queries():
    assert QUERIES == {'q': 'SELECT 42\n'}
