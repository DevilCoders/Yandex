from starlette import status

from api import models
from api.client import ModelBasedHttpClient
from database.models import ClusterDeployPolicyType


def create_test_recipe(client: ModelBasedHttpClient) -> models.ClusterRecipe:
    return client.post(
        "/recipes",
        json=models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="/recipes/1",
            name="meeseeks",
            description="I'm Mr. Meeseeks! Look at me!"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse
    ).recipe


def test_cluster_with_deploy_policy(client: ModelBasedHttpClient, mocked_s3):
    recipe = create_test_recipe(client)

    # 1. create cluster with deploy policy
    cluster = client.post(
        "/clusters",
        json=models.CreateClusterRequest(
            recipe_id=recipe.id,
            name="Meeseeks",
            slug="meeseeks",
            manually_created=True,
            arcadia_path="/clusters/1",
            variables={
                "test-variable": "value"
            },
            deploy_policy=models.ManualClusterDeployPolicy(
                parallelism=20,
                ship=True,
                plan_timeout=3600,
                apply_timeout=3600,
            ),
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterResponse,
    ).cluster

    cluster_list = client.get("/clusters", response_model=models.ClusterListResponse)

    assert len(cluster_list.clusters) == 1
    assert cluster_list.clusters[0].deploy_policy.parallelism == 20
    assert cluster_list.clusters[0].deploy_policy.type == ClusterDeployPolicyType.MANUAL
    assert cluster_list.clusters[0].deploy_policy.ship
    assert cluster_list.clusters[0].deploy_policy.plan_timeout == 3600
    assert cluster_list.clusters[0].deploy_policy.apply_timeout == 3600

    # 2. update cluster with deploy policy
    cluster = client.put(
        f"/clusters/{cluster.id}",
        json=models.CreateClusterRequest(
            recipe_id=recipe.id,
            name="Meeseeks",
            slug="meeseeks",
            manually_created=True,
            arcadia_path="/clusters/1",
            variables={
                "test-variable": "value"
            },
            deploy_policy=models.RegularClusterDeployPolicy(
                parallelism=10,
                sleep_interval=180,
                plan_timeout=3600,
                apply_timeout=3600,
            ),
        ),
        expected_status_code=status.HTTP_200_OK,
        response_model=models.CreateClusterResponse,
    ).cluster

    cluster_list = client.get("/clusters", response_model=models.ClusterListResponse)
    assert len(cluster_list.clusters) == 1

    assert cluster_list.clusters[0].deploy_policy.parallelism == 10
    assert cluster_list.clusters[0].deploy_policy.sleep_interval == 180
    assert cluster_list.clusters[0].deploy_policy.type == ClusterDeployPolicyType.REGULAR
    assert cluster_list.clusters[0].deploy_policy.plan_timeout == 3600
    assert cluster_list.clusters[0].deploy_policy.apply_timeout == 3600

    # 3. remove deploy policy from cluster
    cluster = client.put(
        f"/clusters/{cluster.id}",
        json=models.CreateClusterRequest(
            recipe_id=recipe.id,
            name="Meeseeks",
            slug="meeseeks",
            manually_created=True,
            arcadia_path="/clusters/1",
            variables={
                "test-variable": "value"
            },
            deploy_policy=None,
        ),
        expected_status_code=status.HTTP_200_OK,
        response_model=models.CreateClusterResponse,
    ).cluster

    cluster = client.get(f"/clusters/{cluster.id}", response_model=models.ClusterResponse).cluster
    assert cluster.deploy_policy is None
