import os

import boto3
import moto
import pytest
from sqlalchemy.orm import Session

import database
import settings


@pytest.fixture
def test_database(request):
    database_filename = f"{request.node.name}.sqlite"
    database.connect(f"sqlite:///{database_filename}", {"check_same_thread": False})
    database.Base.metadata.create_all(bind=database.engine)

    try:
        yield
    finally:
        os.unlink(database_filename)


def override_db():
    session = database.session_maker()
    try:
        yield session
    finally:
        session.close()


@pytest.fixture
def db(test_database) -> Session:
    session = database.session_maker()
    try:
        yield session
    finally:
        session.close()


@pytest.fixture
def mocked_s3():
    with moto.mock_s3():
        # Moto works only with default S3 endpoint
        settings.S3_ENDPOINT = None
        # Moto+boto3 work only with non-empty S3_ACCESS_KEY and non-empty S3_SECRET_ACCESS_KEY
        settings.S3_ACCESS_KEY_ID = "mocked-access-key-id"
        settings.S3_SECRET_ACCESS_KEY = "mocked-secret-access-key"

        # Create empty bucket in mocked S3 environment
        s3 = boto3.resource("s3", region_name="us-east-1")
        s3.create_bucket(Bucket=settings.AGENT_CONFIGURATIONS_S3_BUCKET)
        s3.create_bucket(Bucket=settings.MR_PROBER_LOGS_S3_BUCKET)

        yield s3
