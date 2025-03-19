import datetime

import pytz
import sqlalchemy
from sqlalchemy.orm import Session
from starlette import status

import database.models
from api import models
from api.client import ModelBasedHttpClient
from api.tests.common import create_test_recipe_and_cluster


def test_empty_cluster_list(client: ModelBasedHttpClient):
    response = client.get("/clusters", response_model=models.ClusterListResponse)
    assert response.status == "ok"
    assert len(response.clusters) == 0, "Cluster list should be empty on start"


def test_non_authenticated_user_can_not_create_cluster(non_authenticated_client: ModelBasedHttpClient):
    non_authenticated_client.post(
        "/clusters",
        json=models.CreateClusterRequest(recipe_id=1, name="test", slug="test", manually_created=True, variables=[]),
        expected_status_code=status.HTTP_401_UNAUTHORIZED
    )


def test_cluster_needs_recipe(client: ModelBasedHttpClient):
    response = client.post(
        "/clusters",
        json=models.CreateClusterRequest(recipe_id=1, name="test", slug="test", manually_created=True, variables=[]),
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
        response_model=models.ErrorResponse
    )

    assert response.message == "Recipe 1 not found", "Invalid error message"


def test_cluster_is_created_as_manually_created_by_default(client: ModelBasedHttpClient, mocked_s3):
    recipe = client.post(
        "/recipes",
        json=models.CreateClusterRecipeRequest(arcadia_path="", name="test", description="Test"),
        response_model=models.CreateClusterRecipeResponse,
    ).recipe
    assert recipe.manually_created

    cluster = client.post(
        "/clusters",
        json=models.CreateClusterRequest(recipe_id=recipe.id, name="test", slug="test", variables=[]),
        response_model=models.CreateClusterResponse,
    ).cluster
    assert cluster.manually_created


def test_create_and_list_clusters(client: ModelBasedHttpClient, mocked_s3):
    create_test_recipe_and_cluster(client)

    # 1. Check that cluster exists in /clusters
    cluster_list = client.get("/clusters", response_model=models.ClusterListResponse)

    assert len(cluster_list.clusters) == 1
    assert cluster_list.clusters[0].name == "Meeseeks"
    assert cluster_list.clusters[0].slug == "meeseeks"
    assert cluster_list.clusters[0].recipe.name == "meeseeks"
    assert not cluster_list.clusters[0].manually_created
    assert cluster_list.clusters[0].arcadia_path == "/clusters/1"
    assert cluster_list.clusters[0].last_deploy_attempt_finish_time is None
    assert len(cluster_list.clusters[0].variables) == 1
    assert cluster_list.clusters[0].variables[0].name == "test-variable"
    assert cluster_list.clusters[0].variables[0].value == "value"

    # 2. Check that cluster is available on /clusters/1
    response = client.get(f"/clusters/{cluster_list.clusters[0].id}", response_model=models.ClusterResponse)

    assert response.cluster.id == cluster_list.clusters[0].id
    assert response.cluster.recipe.name == "meeseeks"
    assert len(response.cluster.variables) == 1


def test_update_cluster(client: ModelBasedHttpClient, mocked_s3, db: Session):
    recipe, cluster = create_test_recipe_and_cluster(client)

    # 1. Update the cluster
    response = client.put(
        f"/clusters/{cluster.id}", json=models.UpdateClusterRequest(
            recipe_id=recipe.id,
            arcadia_path=recipe.arcadia_path,
            name="New Value",
            slug="new-value",
            variables={
                "new-key": "new-variable",
            }
        ), expected_status_code=status.HTTP_200_OK, response_model=models.UpdateClusterResponse
    )

    # 2. Check returned value
    assert response.cluster.name == "New Value"
    assert response.cluster.slug == "new-value"
    assert len(response.cluster.variables) == 1
    assert response.cluster.variables[0].name == "new-key"
    assert response.cluster.variables[0].value == "new-variable"

    # 3. Check that value has been updated on /clusters/1
    new_cluster = client.get(f"/clusters/{cluster.id}", response_model=models.ClusterResponse).cluster
    assert new_cluster.name == "New Value"
    assert new_cluster.slug == "new-value"
    assert len(new_cluster.variables) == 1
    assert new_cluster.variables[0].name == "new-key"
    assert new_cluster.variables[0].value == "new-variable"

    # 4. Check that recipe id has not changed
    assert new_cluster.recipe.id == recipe.id

    # 5. Check that no orphans left in the database
    assert db.query(database.models.ClusterVariable).count() == 1


def test_not_empty_last_deploy_attempt_finish_time(client: ModelBasedHttpClient, mocked_s3, db: Session):
    # 1. Create a cluster
    _, cluster = create_test_recipe_and_cluster(client)

    # 2. Modify cluster.last_deploy_attempt_finish_time directly in database
    now = datetime.datetime.now(pytz.utc)
    db.execute(
        sqlalchemy.update(database.models.Cluster)
            .where(database.models.Cluster.id == cluster.id)
            .values(last_deploy_attempt_finish_time=now)
    )
    db.commit()

    # 3. Check that API returns new value
    new_cluster = client.get(f"/clusters/{cluster.id}", response_model=models.ClusterResponse).cluster

    assert new_cluster.last_deploy_attempt_finish_time is not None
    assert pytz.utc.localize(new_cluster.last_deploy_attempt_finish_time) == now
