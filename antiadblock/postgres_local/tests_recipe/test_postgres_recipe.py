import os

import pytest

from sqlalchemy import create_engine


@pytest.yield_fixture(scope='session')
def db_connection():
    user = os.environ['PG_LOCAL_USER']
    password = os.environ['PG_LOCAL_PASSWORD']
    db_name = os.environ['PG_LOCAL_DATABASE']
    port = os.environ['PG_LOCAL_PORT']

    db_uri = 'postgresql+psycopg2://{user}:{password}@{host}:{port}/{db_name}'.format(user=user,
                                                                                      password=password,
                                                                                      host='localhost',
                                                                                      port=port,
                                                                                      db_name=db_name)
    engine = None
    try:
        engine = create_engine(db_uri, echo=True)
        with engine.connect() as conn:
            yield conn
    finally:
        if engine is not None:
            engine.dispose()


def test_database(db_connection):
    create_sql = """CREATE TABLE users(
        id BIGSERIAL PRIMARY KEY,
        name VARCHAR(64) NOT NULL
    )
    """
    db_connection.execute(create_sql)
    insert_sql = "INSERT INTO users(name) VALUES ('user1'), ('user2')"
    db_connection.execute(insert_sql)

    select_sql = "SELECT name FROM users"
    results = db_connection.execute(select_sql)

    names = [row[0] for row in results]
    assert 'user1' in names
    assert 'user2' in names


def test_db_is_still_working(db_connection):
    select_sql = "SELECT name FROM users"
    results = db_connection.execute(select_sql)

    names = [row[0] for row in results]
    assert 'user1' in names
    assert 'user2' in names
