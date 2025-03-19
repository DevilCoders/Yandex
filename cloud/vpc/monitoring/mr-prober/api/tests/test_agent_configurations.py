import pytest
from starlette import status

import settings
from api import models
from api.client import ModelBasedHttpClient
from api.tests.common import create_test_prober, create_and_upload_test_prober_file, check_cluster_is_uploaded_to_s3, \
    check_prober_is_uploaded_to_s3


def create_test_cluster(client: ModelBasedHttpClient) -> models.Cluster:
    # 1. Create recipe
    recipe = client.post(
        "/recipes",
        json=models.CreateClusterRecipeRequest(
            arcadia_path="/recipes/1",
            name="meeseeks",
            description="I'm Mr. Meeseeks! Look at me!"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse
    ).recipe

    # 2. Create cluster from this recipe
    return client.post(
        "/clusters",
        json=models.CreateClusterRequest(
            recipe_id=recipe.id,
            name="Meeseeks",
            slug="meeseeks",
            arcadia_path="/clusters/1",
            variables={
                "test-variable": "value"
            },
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterResponse,
    ).cluster


def test_agents_sync_for_empty_configuration(client: ModelBasedHttpClient):
    client.get("/agents/sync", response_model=models.SuccessResponse)


def test_single_cluster_config_is_uploaded_to_s3(client: ModelBasedHttpClient, mocked_s3):
    cluster = create_test_cluster(client)
    check_cluster_is_uploaded_to_s3(mocked_s3, cluster)


def test_agents_sync_works_for_single_cluster(client: ModelBasedHttpClient, mocked_s3):
    cluster = create_test_cluster(client)

    # Delete file from S3
    mocked_s3.Object(
        bucket_name=settings.AGENT_CONFIGURATIONS_S3_BUCKET,
        key=f"{settings.S3_PREFIX}clusters/{cluster.id}/cluster.json"
    ).delete()

    # Check that cluster is not uploaded into S3 (because file is deleted)
    with pytest.raises(mocked_s3.meta.client.exceptions.NoSuchKey):
        check_cluster_is_uploaded_to_s3(mocked_s3, cluster)

    # Call explicit sync for agents configuration
    client.get("/agents/sync")

    check_cluster_is_uploaded_to_s3(mocked_s3, cluster)


def test_agents_sync_works_for_probers(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create cluster, prober and file for it
    create_test_cluster(client)
    prober = create_test_prober(client)
    create_and_upload_test_prober_file(client, prober)
    # Re-download prober because file has been added
    prober = client.get(f"/probers/{prober.id}", response_model=models.ProberResponse).prober

    # 2. Check that prober is uploaded to S3 with file
    check_prober_is_uploaded_to_s3(mocked_s3, client, prober)

    # 3. Delete all content related to the prober from S3
    bucket = mocked_s3.Bucket(settings.AGENT_CONFIGURATIONS_S3_BUCKET)
    bucket.objects.filter(Prefix=f"{settings.S3_PREFIX}probers/{prober.id}/").delete()

    # Check that prober is not uploaded into S3 (because file is deleted)
    with pytest.raises(mocked_s3.meta.client.exceptions.NoSuchKey):
        check_prober_is_uploaded_to_s3(mocked_s3, client, prober)

    # 4. Call explicit sync for agents configuration
    client.get("/agents/sync")

    check_prober_is_uploaded_to_s3(mocked_s3, client, prober)


def test_agents_sync_works_for_probers_with_files_without_uploaded_content(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create cluster and prober
    create_test_cluster(client)
    prober = create_test_prober(client)

    # 2. Create a prober file, but don't upload it's content
    client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(relative_file_path="dns.sh"),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberFileResponse
    )

    # 3. Delete all content related to the prober from S3
    bucket = mocked_s3.Bucket(settings.AGENT_CONFIGURATIONS_S3_BUCKET)
    bucket.objects.filter(Prefix=f"{settings.S3_PREFIX}probers/{prober.id}/").delete()

    # 4. Call explicit sync for agents configuration, it should not fail
    client.get("/agents/sync")

    # 5. And it should not create any files also
    assert len(list(bucket.objects.filter(Prefix=f"{settings.S3_PREFIX}probers/{prober.id}/files/"))) == 0
