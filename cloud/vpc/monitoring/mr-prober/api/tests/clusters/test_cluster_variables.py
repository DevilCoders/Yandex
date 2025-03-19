from typing import Any

import pytest
from starlette import status

from api.client import ModelBasedHttpClient
from api import models
from api.tests.common import create_test_recipe_and_cluster, check_cluster_is_uploaded_to_s3


def test_add_variable(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)

    response = client.post(
        f"/clusters/{cluster.id}/variables",
        json=models.CreateClusterVariableRequest(
            name="new-variable",
            value="variable-value",
        ),
        response_model=models.CreateClusterVariableResponse,
        expected_status_code=status.HTTP_201_CREATED,
    )

    assert response.variable.name == "new-variable"
    assert response.variable.value == "variable-value"

    assert response.variable in response.cluster.variables

    cluster = client.get(f"/clusters/{cluster.id}", response_model=models.ClusterResponse).cluster
    check_cluster_is_uploaded_to_s3(mocked_s3, cluster)


def test_add_variable_with_same_name(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)

    client.post(
        f"/clusters/{cluster.id}/variables",
        json=models.CreateClusterVariableRequest(
            name="test-variable",
            value="new-value",
        ),
        expected_status_code=status.HTTP_409_CONFLICT,
    )


def update_cluster_variable_value(
    client: ModelBasedHttpClient, cluster: models.Cluster, variable: models.ClusterVariable, new_value: Any
) -> models.ClusterVariable:
    response = client.put(
        f"/clusters/{cluster.id}/variables/{variable.id}",
        json=models.UpdateClusterVariableRequest(
            name="test-variable",
            value=new_value,
        ),
        response_model=models.UpdateClusterVariableResponse,
    )
    assert response.variable.value == new_value

    return response.variable


def test_update_variable(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)

    variable = update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


# Variable-Value-as-List/Variable-Value-as-Set modifications

@pytest.mark.parametrize("variable_type", ("list", "set"))
def test_update_variable_add_element_to_it(client: ModelBasedHttpClient, mocked_s3, variable_type: str):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    response = client.put(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/{variable_type}",
        json=models.UpdateClusterVariableAsListOrSetRequest(
            value=10,
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == [1, 2, 3, 10]

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


@pytest.mark.parametrize("variable_type", ("list", "set"))
def test_update_variable_add_element_to_it_but_wrong_type(client: ModelBasedHttpClient, mocked_s3, variable_type: str):
    _recipe, cluster = create_test_recipe_and_cluster(client)

    response = client.put(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/{variable_type}",
        json=models.UpdateClusterVariableAsListOrSetRequest(
            value=10,
        ),
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
        response_model=models.ErrorResponse,
    )
    assert "current value is not a list" in response.message


def test_update_variable_remove_element_from_list(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/list",
        json=models.UpdateClusterVariableDeleteFromListRequest(
            index=1,
            value=2,
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == [1, 3]

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


def test_update_variable_remove_element_from_list_but_wrong_type(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/list",
        json=models.UpdateClusterVariableDeleteFromListRequest(
            index=1,
            value=20,
        ),
        response_model=models.ErrorResponse,
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED
    )
    assert "current value is not a list" in response.message


def test_update_variable_remove_element_from_list_but_wrong_value(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/list",
        json=models.UpdateClusterVariableDeleteFromListRequest(
            index=1,
            value=20,
        ),
        response_model=models.ErrorResponse,
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED
    )
    assert "variable has been changed by someone else" in response.message


@pytest.mark.parametrize("index", (-1, 3, 100))
def test_update_variable_remove_element_from_list_but_wrong_index(client: ModelBasedHttpClient, mocked_s3, index: int):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/list",
        json=models.UpdateClusterVariableDeleteFromListRequest(
            index=index,
            value=20,
        ),
        response_model=models.ErrorResponse,
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED
    )
    assert "out of range" in response.message


# Variable-Value-as-Set modifications

def test_update_variable_add_element_to_set_but_value_already_exists(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    response = client.put(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/set",
        json=models.UpdateClusterVariableAsListOrSetRequest(
            value=1,
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == [1, 2, 3]

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


def test_update_variable_delete_element_from_set(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], [1, 2, 3])

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/set",
        json=models.UpdateClusterVariableAsListOrSetRequest(
            value=2,
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == [1, 3]

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


# Variable-Value-as-Map modifications

def test_update_variable_add_element_to_map(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], {"test-key-1": "value", "test-key-2": 100500})

    response = client.put(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/map",
        json=models.UpdateClusterVariableAsMapRequest(
            key="test-key-3",
            value="new-value",
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == {"test-key-1": "value", "test-key-2": 100500, "test-key-3": "new-value"}

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


def test_update_variable_update_element_on_map(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], {"test-key-1": "value", "test-key-2": 100500})

    response = client.put(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/map",
        json=models.UpdateClusterVariableAsMapRequest(
            key="test-key-1",
            value="new-value",
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == {"test-key-1": "new-value", "test-key-2": 100500}

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


def test_update_variable_delete_element_from_map(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], {"test-key-1": "value", "test-key-2": 100500})

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/map",
        json=models.UpdateClusterVariableAsMapRequest(
            key="test-key-1",
            value="value",
        ),
        response_model=models.UpdateClusterVariableResponse,
    )

    assert response.variable.value == {"test-key-2": 100500}

    new_cluster = client.get(f"/clusters/{cluster.id}/", response_model=models.ClusterResponse).cluster
    assert new_cluster.variables == [response.variable]

    check_cluster_is_uploaded_to_s3(mocked_s3, new_cluster)


def test_update_variable_delete_element_from_map_but_wrong_value(client: ModelBasedHttpClient, mocked_s3):
    _recipe, cluster = create_test_recipe_and_cluster(client)
    update_cluster_variable_value(client, cluster, cluster.variables[0], {"test-key-1": "value", "test-key-2": 100500})

    response = client.delete(
        f"/clusters/{cluster.id}/variables/{cluster.variables[0].id}/map",
        json=models.UpdateClusterVariableAsMapRequest(
            key="test-key-1",
            value="other-value",
        ),
        response_model=models.ErrorResponse,
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
    )

    assert "has been changed" in response.message
