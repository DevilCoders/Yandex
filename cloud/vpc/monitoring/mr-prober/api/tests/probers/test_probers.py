from starlette import status

from api import models
from api.client import ModelBasedHttpClient
from api.tests.common import create_test_prober, create_and_upload_test_prober_file


def test_create_prober(client: ModelBasedHttpClient, mocked_s3):
    """
    Just create a new prober and check it in the list
    """
    # 1. Create prober
    prober = create_test_prober(client)

    # 2. Check that prober is available in the list on /probers
    prober_list = client.get("/probers", response_model=models.ProberListResponse)

    assert len(prober_list.probers) == 1
    assert prober_list.probers[0].name == "dns"
    assert prober_list.probers[0].slug == "dns"
    assert prober_list.probers[0].description == "External DNS request"
    assert prober_list.probers[0].manually_created
    assert prober_list.probers[0].arcadia_path == "/probers/1"
    assert prober_list.probers[0].runner.type == "BASH"
    assert prober_list.probers[0].runner.command == "/bin/bash -c ./prober-files/dns.sh"

    # 3. Check that prober is available on /probers/1
    response = client.get(f"/probers/{prober.id}", response_model=models.ProberResponse)
    assert response.prober == prober_list.probers[0]


def test_create_prober_without_runner(client: ModelBasedHttpClient, mocked_s3):
    client.post(
        "/probers",
        json={
            "name": "dns",
            "slug": "dns",
            "description": "External DNS request",
            "manually_created": True,
            "arcadia_path": "/probers/1",
        },
        expected_status_code=status.HTTP_400_BAD_REQUEST,
    )


def test_create_prober_with_unknown_runner_type(client: ModelBasedHttpClient, mocked_s3):
    client.post(
        "/probers",
        json={
            "name": "dns",
            "slug": "dns",
            "description": "External DNS request",
            "manually_created": True,
            "arcadia_path": "/probers/1",
            "runner": {
                "type": "UNKNOWN",
                "key": "value",
            }
        },
        expected_status_code=status.HTTP_400_BAD_REQUEST,
    )


def test_create_prober_copy(client: ModelBasedHttpClient, mocked_s3):
    """
    Prober can be copied from another prober
    """
    prober = create_test_prober(client)

    response = client.post(
        "/probers/copy",
        json=models.CopyProberRequest(
            prober_id=prober.id
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberResponse
    )
    copied_prober = response.prober

    assert copied_prober.id != prober.id, "Copied prober should have another id"
    assert copied_prober.name == prober.name
    assert copied_prober.slug == prober.slug
    assert copied_prober.description == prober.description
    assert copied_prober.arcadia_path == prober.arcadia_path
    assert copied_prober.runner.command == prober.runner.command

    prober_list = client.get("/probers", response_model=models.ProberListResponse)
    assert len(prober_list.probers) == 2


def test_create_prober_copy_with_files(client: ModelBasedHttpClient, mocked_s3):
    """
    Prober should be copied with its files
    """
    prober = create_test_prober(client)
    file = create_and_upload_test_prober_file(client, prober)

    response = client.post(
        "/probers/copy",
        json=models.CopyProberRequest(
            prober_id=prober.id
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberResponse
    )
    copied_prober = response.prober
    assert len(copied_prober.files) == 1
    assert copied_prober.files[0].id != file.id, "copied file should have another id"

    prober_list = client.get("/probers", response_model=models.ProberListResponse)
    assert len(prober_list.probers) == 2
    for prober in prober_list.probers:
        assert len(prober.files) == 1


def test_create_prober_copy_without_configs(client: ModelBasedHttpClient, mocked_s3):
    """
    Prober should be copied without its configs
    """
    prober = create_test_prober(client)
    client.post(
        f"/probers/{prober.id}/configs",
        json=models.CreateProberConfigRequest(
            manually_created=True,
            is_prober_enabled=True,
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberConfigResponse
    )

    response = client.post(
        "/probers/copy",
        json=models.CopyProberRequest(
            prober_id=prober.id
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberResponse
    )
    copied_prober = response.prober
    assert len(copied_prober.configs) == 0

    prober_list = client.get("/probers", response_model=models.ProberListResponse)
    assert len(prober_list.probers) == 2
    assert len(prober_list.probers[0].configs) == 1
    assert len(prober_list.probers[1].configs) == 0


def test_update_prober(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create prober
    prober = create_test_prober(client)

    # 2. Update prober, change its name and slug
    response = client.put(
        f"/probers/{prober.id}",
        json=models.UpdateProberRequest(
            manually_created=True,
            arcadia_path="/probers/1",
            name="DNS checker",
            slug="dns_checker",
            description="External DNS request",
            runner=models.BashProberRunner(
                command="./dns.sh",
            ),
        ),
        expected_status_code=status.HTTP_200_OK,
        response_model=models.UpdateProberResponse
    )

    updated_prober = response.prober

    assert updated_prober.name == "DNS checker"
    assert updated_prober.slug == "dns_checker"
    assert updated_prober.description == "External DNS request"
    assert updated_prober.manually_created
    assert updated_prober.arcadia_path == "/probers/1"
    assert updated_prober.runner.command == "./dns.sh"

    # 3. Check that prober is updated on /probers/1
    response = client.get(f"/probers/{prober.id}", response_model=models.ProberResponse)
    assert response.prober == updated_prober


def test_prober_not_found(client: ModelBasedHttpClient):
    client.get(f"/probers/20", expected_status_code=status.HTTP_404_NOT_FOUND)


def test_delete_prober(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create prober
    prober = create_test_prober(client)

    # 2. Check that prober exists
    client.get(f"/probers/{prober.id}", expected_status_code=status.HTTP_200_OK)

    # 3. Delete prober
    client.delete(f"/probers/{prober.id}", response_model=models.SuccessResponse)

    # 4. Check that prober doesn't exist
    client.get(f"/probers/{prober.id}", expected_status_code=status.HTTP_404_NOT_FOUND)

    # 5. Check that prober doesn't exist in list /probers
    response = client.get(f"/probers", response_model=models.ProberListResponse)
    assert len(response.probers) == 0
