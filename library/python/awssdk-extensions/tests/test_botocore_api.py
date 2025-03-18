# boto3 import is required for botocore to have `session` attribute
import boto3  # noqa
import botocore


def test_session_credentials_attribute():
    session = botocore.session.get_session()
    assert hasattr(session, '_credentials')
