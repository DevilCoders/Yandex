import pathlib

import database.models
import tools.synchronize.data.models as data_models
from api.client import MrProberApiClient, ModelBasedHttpClient
import api


def test_cluster_loaded_from_iac(testdata):
    cluster = data_models.Cluster.load_from_iac(pathlib.Path("clusters/prod/meeseeks/cluster.yaml"))

    assert cluster.id is None
    assert cluster.name == "Meeseeks"
    assert cluster.slug == "meeseeks"
    assert len(cluster.variables) == 1
    assert cluster.variables["cluster_id"] == 1
    assert cluster.deploy_policy.parallelism == 20
    assert not cluster.deploy_policy.ship
    assert cluster.deploy_policy.type == database.models.ClusterDeployPolicyType.MANUAL
    assert cluster.deploy_policy.plan_timeout == 180
    assert cluster.deploy_policy.apply_timeout == 1800


def test_cluster_slug_is_loaded_if_specified(testdata):
    cluster = data_models.Cluster.load_from_iac(pathlib.Path("clusters/some-environment/slug/cluster.yaml"))

    assert cluster.slug == "test"


def test_untracked_variables_loaded_if_specified(testdata):
    cluster = data_models.Cluster.load_from_iac(
        pathlib.Path("clusters/some-environment/iac-untracked-variables/cluster.yaml")
    )

    assert cluster.untracked_variables == ["compute_nodes"]


def test_different_variable_types(testdata):
    cluster = data_models.Cluster.load_from_iac(pathlib.Path("clusters/some-environment/variables/cluster.yaml"))

    assert type(cluster.variables["integer"]) is int
    assert type(cluster.variables["zero"]) is int
    assert type(cluster.variables["float"]) is float
    assert type(cluster.variables["boolean"]) is bool
    assert type(cluster.variables["boolean_yes"]) is bool
    assert type(cluster.variables["string_without_quotes"]) is str
    assert type(cluster.variables["string_with_quotes"]) is str
    assert type(cluster.variables["string_true"]) is str

    assert cluster.variables["integer"] == 1
    assert cluster.variables["zero"] == 0
    assert cluster.variables["string_without_quotes"] == "value"
    assert cluster.variables["string_with_quotes"] == "hello world"
    assert cluster.variables["string_empty"] == ""
    assert cluster.variables["float"] == 1.5
    assert cluster.variables["boolean"] == False
    assert cluster.variables["boolean_yes"] == True
    assert cluster.variables["string_true"] == "true"
    assert cluster.variables["list"] == ["first", "second", 10, True]
    assert cluster.variables["dict"] == {
        "string_key": "value",
        "list_key": ["first", "second"],
    }


def test_cluster_loaded_from_api(client: ModelBasedHttpClient, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)
    recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="recipes/meeseeks/recipe.yaml",
            name="Meeseeks",
            description="Meeseeks cluster recipe",
        )
    )
    cluster = api_client.clusters.create(
        api.models.CreateClusterRequest(
            recipe_id=recipe.id,
            name="Meeseeks",
            slug="meeseeks",
            manually_created=True,
            arcadia_path="clusters/prod/meeseeks/cluster.yaml",
            variables={
                "key": "value"
            },
            deploy_policy=api.models.RegularClusterDeployPolicy(
                parallelism=20,
                sleep_interval=180,
                plan_timeout=360,
                apply_timeout=3600,
            ),
        )
    )

    loaded_cluster = data_models.Cluster.load_from_api(api_client, cluster.id)
    assert loaded_cluster.id == cluster.id
    assert loaded_cluster.name == "Meeseeks"
    assert loaded_cluster.slug == "meeseeks"
    assert loaded_cluster.arcadia_path == "clusters/prod/meeseeks/cluster.yaml"
    assert loaded_cluster.variables == {
        "key": "value"
    }

    assert loaded_cluster.deploy_policy.parallelism == 20
    assert loaded_cluster.deploy_policy.sleep_interval == 180
    assert loaded_cluster.deploy_policy.type == database.models.ClusterDeployPolicyType.REGULAR
    assert loaded_cluster.deploy_policy.plan_timeout == 360
    assert loaded_cluster.deploy_policy.apply_timeout == 3600

    assert loaded_cluster.recipe.id == recipe.id
    assert loaded_cluster.recipe.arcadia_path == "recipes/meeseeks/recipe.yaml"
    assert loaded_cluster.recipe.name == "Meeseeks"
    assert loaded_cluster.recipe.description == "Meeseeks cluster recipe"
