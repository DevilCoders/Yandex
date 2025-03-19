import io

from starlette import status

from api import models
from api.client import ModelBasedHttpClient
from api.tests.common import (
    create_test_prober,
    create_test_prober_config,
    create_and_upload_test_prober_file,

    testdata_create_prober_file_request,
    testdata_prober_file_content,
)


def test_create_prober_file(client: ModelBasedHttpClient, mocked_s3):
    """
    Prober's file content should be downloaded
    """
    prober = create_test_prober(client)
    file = create_and_upload_test_prober_file(client, prober)

    assert file.relative_file_path == testdata_create_prober_file_request.relative_file_path
    assert file.is_executable
    assert file.md5_hexdigest == "d0972fe1d535fdad5fbab75321b0ccdb"

    response = client.get(
        f"/probers/{prober.id}/files/{file.id}/content",
    )

    assert response.content == testdata_prober_file_content.encode()


def test_create_prober_file_with_wrong_filepath(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)

    client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(
            manually_created=True,
            is_executable=False,
            relative_file_path=".//../dns.sh",
        ),
        expected_status_code=status.HTTP_400_BAD_REQUEST,
    )


def test_create_prober_file_with_duplicate_filepath(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    create_and_upload_test_prober_file(client, prober)

    client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(
            manually_created=True,
            is_executable=False,
            relative_file_path="dns.sh",
        ),
        expected_status_code=status.HTTP_409_CONFLICT,
    )


def test_update_prober_file_with_duplicate_filepath(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    create_and_upload_test_prober_file(client, prober)

    response = client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(
            manually_created=True,
            is_executable=False,
            relative_file_path="metadata.sh",
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberFileResponse
    )
    assert not response.file.is_executable

    client.put(
        f"/probers/{prober.id}/files/{response.file.id}",
        json=models.UpdateProberFileRequest(
            manually_created=True,
            is_executable=False,
            relative_file_path="dns.sh",
        ),
        expected_status_code=status.HTTP_409_CONFLICT,
    )


def test_create_prober_file_is_prohibited_if_prober_has_configs(client: ModelBasedHttpClient, mocked_s3):
    """
    Creating file for prober with config should fail with 412 PRECONDITION FAILED
    """
    prober = create_test_prober(client)
    create_test_prober_config(client, prober)

    client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(
            manually_created=True,
            is_executable=False,
            relative_file_path="dns.sh",
        ),
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
    )


def test_create_prober_file_is_not_prohibited_if_prober_has_configs_but_force_specified(
    client: ModelBasedHttpClient, mocked_s3
):
    """
    Creating file for prober with config should NOT fail with 412 PRECONDITION FAILED
    if {..., "force": True} is specified in request body.
    """
    prober = create_test_prober(client)
    create_test_prober_config(client, prober)

    client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(
            manually_created=True,
            is_executable=False,
            relative_file_path="dns.sh",
            force=True,
        ),
        expected_status_code=status.HTTP_201_CREATED,
    )


def test_upload_prober_file_is_prohibited_if_prober_has_configs(client: ModelBasedHttpClient, mocked_s3):
    """
    Uploading file for prober with config should fail with 412 PRECONDITION FAILED
    """
    prober = create_test_prober(client)
    file = create_and_upload_test_prober_file(client, prober)
    create_test_prober_config(client, prober)

    client.put(
        f"/probers/{prober.id}/files/{file.id}/content",
        files={"content": ("filename", io.StringIO("dig yandex.ru"), "text/plain")},
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
    )


def test_upload_prober_file_is_not_prohibited_if_prober_has_configs_but_force_specified(
    client: ModelBasedHttpClient, mocked_s3
):
    """
    Uploading file for prober with config should NOT fail with 412 PRECONDITION FAILED
    if force=True is specified in query string.
    """
    prober = create_test_prober(client)
    file = create_and_upload_test_prober_file(client, prober)
    create_test_prober_config(client, prober)

    client.put(
        f"/probers/{prober.id}/files/{file.id}/content?force=True",
        files={"content": ("filename", io.StringIO("dig yandex.ru"), "text/plain")},
        expected_status_code=status.HTTP_200_OK,
        response_model=models.ProberFileResponse,
    )


def test_cant_download_file_content_until_it_is_uploaded(client: ModelBasedHttpClient, mocked_s3):
    """
    Downloading file for prober should fail with 404 NOT FOUND, if it hasn't been uploaded yet
    """
    prober = create_test_prober(client)

    # Create a prober file, but don't upload it
    file = client.post(
        f"/probers/{prober.id}/files",
        json=models.CreateProberFileRequest(relative_file_path="dns.sh"),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberFileResponse
    ).file

    client.get(f"/probers/{prober.id}/files/{file.id}/content", expected_status_code=status.HTTP_404_NOT_FOUND)
