import io
import json
from typing import Tuple

from starlette import status

import settings
from api import models
from api.client import ModelBasedHttpClient

__all__ = [
    "create_and_upload_test_prober_file",
    "create_test_prober",
    "create_test_prober_config",
    "create_test_recipe_and_cluster",
    "copy_configs_to_prober",
    "move_configs_to_prober",
    "delete_prober_config",

    "testdata_create_cluster_recipe_request",
    "testdata_create_cluster_request",
    "testdata_create_prober_config_request",
    "testdata_create_prober_config_with_variables_request",
    "testdata_prober_file_content",
    "testdata_create_prober_file_request",
    "testdata_create_prober_request_dns",

    "check_cluster_is_uploaded_to_s3",
    "check_prober_is_uploaded_to_s3",
]

testdata_create_prober_request_dns = models.CreateProberRequest(
    name="dns",
    slug="dns",
    description="External DNS request",
    arcadia_path="/probers/1",
    runner=models.BashProberRunner(
        command="/bin/bash -c ./prober-files/dns.sh",
    ),
)

testdata_create_prober_config_request = models.CreateProberConfigRequest(
    manually_created=True,
    is_prober_enabled=True,
    default_routing_interface="eth0",
)

testdata_create_prober_config_with_variables_request = models.CreateProberConfigRequest(
    manually_created=True,
    is_prober_enabled=True,
    matrix_variables={
        "ip_version": [4, 6],
        "host": ["yandex.ru", "google.com"],
    },
    variables={
        "none_var": None,
        "str_var_1": "str_value",
        "str_var_2": "str_value 2",
        "int_var": 1,
        "float_var": 2.54,
        "bool_var": True,
        "dict_var": {"sub_key_1": "value1", "sub_key_2": 2},
        "list_var": ["value1", 2],
        "tuple_var": ("value1", 2),
    }
)

testdata_create_prober_file_request = models.CreateProberFileRequest(
    manually_created=True,
    is_executable=True,
    relative_file_path="dns.sh",
)

testdata_prober_file_content = "dig yandex.ru"

testdata_create_cluster_recipe_request = models.CreateClusterRecipeRequest(
    manually_created=False,
    arcadia_path="/recipes/1",
    name="meeseeks",
    description="I'm Mr. Meeseeks! Look at me!"
)

testdata_create_cluster_request = models.CreateClusterRequest(
    name="Meeseeks",
    slug="meeseeks",
    manually_created=False,
    arcadia_path="/clusters/1",
    variables={
        "test-variable": "value"
    },
    recipe_id=0,
)


def create_test_prober(
    client: ModelBasedHttpClient, create_request: models.CreateProberRequest = testdata_create_prober_request_dns
) -> models.Prober:
    response = client.post(
        "/probers",
        json=create_request,
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberResponse
    )
    prober = response.prober

    assert prober.name == create_request.name
    assert prober.slug == create_request.slug
    assert prober.description == create_request.description
    # Prober should be created as manually_created by default
    assert prober.manually_created
    assert prober.arcadia_path == create_request.arcadia_path
    assert prober.runner.command == create_request.runner.command

    return prober


def create_test_prober_config(
    client: ModelBasedHttpClient, prober: models.Prober,
    create_request: models.CreateProberConfigRequest = testdata_create_prober_config_request
) -> models.ProberConfig:
    response = client.post(
        f"/probers/{prober.id}/configs",
        json=create_request,
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberConfigResponse
    )

    created_config = response.config

    assert created_config.manually_created
    assert created_config.is_prober_enabled
    if create_request.cluster_id:
        assert created_config.cluster_id == create_request.cluster_id

    return created_config


def create_and_upload_test_prober_file(
    client: ModelBasedHttpClient, prober: models.Prober,
    create_request: models.CreateProberFileRequest = testdata_create_prober_file_request,
    content: str = testdata_prober_file_content,
) -> models.ProberFile:
    response = client.post(
        f"/probers/{prober.id}/files",
        json=create_request,
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateProberFileResponse
    )

    assert response.prober.name == prober.name
    assert response.prober.slug == prober.slug
    assert response.prober.description == prober.description
    assert response.prober.manually_created == prober.manually_created
    assert response.prober.arcadia_path == prober.arcadia_path
    assert response.prober.runner.command == prober.runner.command
    assert response.file.relative_file_path == create_request.relative_file_path
    assert response.file.is_executable

    response = client.put(
        f"/probers/{prober.id}/files/{response.file.id}/content",
        files={"content": ("filename", io.StringIO(content), "text/plain")},
        expected_status_code=status.HTTP_200_OK,
        response_model=models.UpdateProberFileResponse
    )

    return response.file


def create_test_recipe_and_cluster(
    client: ModelBasedHttpClient,
    create_recipe_request: models.CreateClusterRecipeRequest = testdata_create_cluster_recipe_request,
    create_cluster_request: models.CreateClusterRequest = testdata_create_cluster_request,
) -> Tuple[models.ClusterRecipe, models.Cluster]:
    # 1. Create recipe
    recipe = client.post(
        "/recipes",
        json=create_recipe_request,
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse
    ).recipe

    create_cluster_request = create_cluster_request.copy(update={"recipe_id": recipe.id})
    # 2. Create cluster from this recipe
    cluster = client.post(
        "/clusters",
        json=create_cluster_request,
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterResponse,
    ).cluster

    return recipe, cluster


def copy_configs_to_prober(
    client: ModelBasedHttpClient, source_prober_id: int, target_prober_id: int
) -> models.Prober:
    client.post(
        f"/probers/{target_prober_id}/configs/copy",
        json=models.CopyOrMoveProberConfigsRequest(
            prober_id=source_prober_id,
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.ProberResponse,
    )

    response = client.get(
        f"/probers/{target_prober_id}",
        response_model=models.ProberResponse,
    )
    return response.prober


def move_configs_to_prober(
    client: ModelBasedHttpClient, source_prober_id: int, target_prober_id: int
) -> models.Prober:
    client.post(
        f"/probers/{target_prober_id}/configs/move",
        json=models.CopyOrMoveProberConfigsRequest(
            prober_id=source_prober_id,
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.ProberResponse,
    )

    response = client.get(
        f"/probers/{target_prober_id}",
        response_model=models.ProberResponse,
    )
    return response.prober


def delete_prober_config(client: ModelBasedHttpClient, prober_id: int, prober_config_id: int) -> models.Prober:
    client.delete(
        f"/probers/{prober_id}/configs/{prober_config_id}",
        expected_status_code=status.HTTP_200_OK,
        response_model=models.DeleteProberConfigResponse,
    )

    response = client.get(
        f"/probers/{prober_id}",
        response_model=models.ProberResponse,
    )
    return response.prober


def get_file_from_agent_configurations_bucket(s3, filename: str) -> str:
    obj = s3.Object(bucket_name=settings.AGENT_CONFIGURATIONS_S3_BUCKET, key=settings.S3_PREFIX + filename)
    return obj.get()["Body"].read().decode()


def check_cluster_is_uploaded_to_s3(s3, cluster: models.Cluster):
    cluster_in_s3 = json.loads(
        get_file_from_agent_configurations_bucket(s3, f"clusters/{cluster.id}/cluster.json")
    )
    assert cluster_in_s3["id"] == cluster.id
    assert cluster_in_s3["name"] == cluster.name
    assert cluster_in_s3["slug"] == cluster.slug
    assert cluster_in_s3["variables"] == {variable.name: variable.value for variable in cluster.variables}


def check_prober_is_uploaded_to_s3(s3, client: ModelBasedHttpClient, prober: models.Prober):
    prober_in_s3 = json.loads(
        get_file_from_agent_configurations_bucket(s3, f"probers/{prober.id}/prober.json")
    )
    assert prober_in_s3["id"] == prober.id
    assert prober_in_s3["name"] == prober.name
    assert prober_in_s3["slug"] == prober.slug
    assert prober_in_s3["runner"]["type"] == prober.runner.type.value
    assert len(prober_in_s3["files"]) == len(prober.files)

    for file_index, file in enumerate(prober.files):
        assert prober_in_s3["files"][file_index]["id"] == file.id
        assert prober_in_s3["files"][file_index]["relative_file_path"] == file.relative_file_path

        original_file_content = client.get(f"/probers/{prober.id}/files/{file.id}/content").text
        file_in_s3 = get_file_from_agent_configurations_bucket(s3, f"probers/{prober.id}/files/{file.id}")
        assert file_in_s3 == original_file_content