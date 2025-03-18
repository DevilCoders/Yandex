import os

import boto3
import pytest


# NB: stub does not check authorization at the time
TEST_ACCESS_KEY_ID = "1234567890"
TEST_SECRET_ACCESS_KEY = "abcdefabcdef"

TEST_BUCKET = "barrel"
TEST_KEY = "the/test/key"
TEST_DATA = b"the-test-data"

TEST_KEY_2 = "the/other/key"
TEST_DATA_2 = b"other-test-data"


@pytest.fixture(scope="function")
def s3_client():
    stub_port = os.environ["S3MDS_PORT"]
    endpoint_url = f"http://localhost:{stub_port}"
    return boto3.resource(
        "s3",
        endpoint_url=endpoint_url,
        aws_access_key_id=TEST_ACCESS_KEY_ID,
        aws_secret_access_key=TEST_SECRET_ACCESS_KEY
    )


def test_creating_bucket(s3_client):
    bucket = s3_client.Bucket(TEST_BUCKET)
    bucket.create()
    bucket.delete()


def test_creating_object(s3_client):
    bucket = s3_client.Bucket(TEST_BUCKET)
    bucket.create()
    obj = s3_client.Object(TEST_BUCKET, TEST_KEY)
    obj.put(Body=TEST_DATA)

    # Somehow Bucket objects does not have get_object method
    received_data = obj.get()["Body"].read()
    assert received_data == TEST_DATA
    obj.delete()
    bucket.delete()


def test_copy_object(s3_client):
    bucket = s3_client.Bucket(TEST_BUCKET)
    bucket.create()
    obj = s3_client.Object(TEST_BUCKET, TEST_KEY)
    obj.put(Body=TEST_DATA)

    other = s3_client.Object(TEST_BUCKET, TEST_KEY_2)
    other.copy_from(CopySource={"Bucket": "barrel", "Key": "the/test/key"})

    received_data = other.get()["Body"].read()
    assert received_data == TEST_DATA
    obj.delete()
    other.delete()
    bucket.delete()


def test_listing_objects(s3_client):
    bucket = s3_client.Bucket(TEST_BUCKET)
    bucket.create()
    obj = s3_client.Object(TEST_BUCKET, TEST_KEY)
    obj.put(Body=TEST_DATA)
    other = s3_client.Object(TEST_BUCKET, TEST_KEY_2)
    other.put(Body=TEST_DATA_2)

    objects = bucket.objects.all()
    assert sorted(list(o.key for o in objects)) == sorted([TEST_KEY, TEST_KEY_2])

    obj.delete()
    other.delete()
    bucket.delete()


def test_listing_objects_with_prefix(s3_client):
    bucket = s3_client.Bucket(TEST_BUCKET)
    bucket.create()
    obj = s3_client.Object(TEST_BUCKET, TEST_KEY)
    obj.put(Body=TEST_DATA)
    other = s3_client.Object(TEST_BUCKET, TEST_KEY_2)
    other.put(Body=TEST_DATA_2)

    objects = list(bucket.objects.filter(Prefix="the/test/"))
    assert len(objects) == 1
    assert objects[0].key == TEST_KEY

    obj.delete()
    other.delete()
    bucket.delete()
