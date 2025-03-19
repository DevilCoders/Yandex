from conftest import DB_VERSION


def test_db_bootstrapped(bootstrap_db):
    assert bootstrap_db.bootstrapped
    assert bootstrap_db.select_all("scheme_info") == [(True, DB_VERSION, None)]
    assert bootstrap_db.url == "postgresql://postgres:PASSWORD_IS_HIDDEN@localhost:12000/postgres"
