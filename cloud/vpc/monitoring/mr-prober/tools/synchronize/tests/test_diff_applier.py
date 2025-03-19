import api.models
from api.client import MrProberApiClient, ModelBasedHttpClient
from api.models import BashProberRunner
from database.models import UploadProberLogPolicy, ClusterDeployPolicyType
from tools.synchronize.data import models as data_models
from tools.synchronize.data.collection import ObjectsCollection
from tools.synchronize.diff import DiffBuilder, DiffApplier

test_recipe_file = data_models.RecipeFile(relative_file_path="instances.tf", content=b"resource {}")

test_recipe = data_models.Recipe(
    id=None,
    arcadia_path="recipes/meeseeks/recipe.yaml",
    name="Meeseeks",
    description="Meeseeks description",
    files=[test_recipe_file],
)

test_deploy_policy = api.models.RegularClusterDeployPolicy(
    parallelism=20,
    sleep_interval=180,
    type=ClusterDeployPolicyType.REGULAR,
    plan_timeout=360,
    apply_timeout=3600,
)

test_cluster = data_models.Cluster(
    id=None,
    arcadia_path="clusters/prod/meeseeks/cluster.yaml",
    name="IaC cluster",
    slug="iac-cluster",
    recipe=test_recipe,
    variables={"key": "values"},
    deploy_policy=test_deploy_policy,
)

test_prober_file = data_models.ProberFile(
    is_executable=True,
    relative_file_path="dns.sh",
    content=b"#!/bin/bash"
)

test_prober_config = data_models.ProberConfig(
    hosts_re="agent-.*",

    interval_seconds=60,
    timeout_seconds=10,
    s3_logs_policy=UploadProberLogPolicy.FAIL,
    variables={"interface": "eth0"},
    matrix={"url": ["https://yandex.ru", "https://google.com"]}
)

test_prober = data_models.Prober(
    id=None,
    arcadia_path="probers/network/dns/prober.yaml",
    name="DNS",
    slug="dns",
    description="Simple DNS check",
    runner=BashProberRunner(
        command="./dns.sh",
    ),
    files=[test_prober_file],
    configs=[test_prober_config],
)


def test_recipe_updated_in_place_if_there_is_no_file_changes(client: ModelBasedHttpClient, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    # 1. Create a local IaC objects collection with one recipe and one cluster
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )

    # 2. Create a recipe in API with "old" name and description, but with same files set
    api_recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=False,
            arcadia_path="recipes/meeseeks/recipe.yaml",
            name="Old Meeseeks",
            description="Old Meeseeks description",
        )
    )
    api_file = api_client.recipes.add_file(
        api_recipe.id, api.models.CreateClusterRecipeFileRequest(
            relative_file_path=test_recipe_file.relative_file_path
        )
    )
    api_client.recipes.upload_file_content(api_recipe.id, api_file.id, test_recipe_file.content)

    # 3. Load objects collection from API
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)

    # 4. And build a diff. Diff should contain only recipe's change
    diff = DiffBuilder().build(iac_collection, api_collection)

    # 5. Apply diff
    DiffApplier().apply(api_client, api_collection, diff)

    # 6. Check that recipe saved their id and changed name and description
    recipe_list = api_client.recipes.list()
    assert len(recipe_list) == 1
    assert recipe_list[0].id == api_recipe.id
    assert recipe_list[0].name == test_recipe.name
    assert recipe_list[0].description == test_recipe.description

    # 7. Cluster has been created from this recipe
    cluster_list = api_client.clusters.list()
    assert len(cluster_list) == 1
    assert cluster_list[0].recipe == recipe_list[0]


def test_recipe_updated_if_there_is_file_change(client: ModelBasedHttpClient, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    # 1. Create a local IaC objects collection with one recipe and one cluster
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )

    # 2. Create a recipe in API with same name and description, ...
    api_recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=False,
            arcadia_path="recipes/meeseeks/recipe.yaml",
            name="Meeseeks",
            description="Meeseeks description",
        )
    )
    api_file = api_client.recipes.add_file(
        api_recipe.id, api.models.CreateClusterRecipeFileRequest(
            relative_file_path=test_recipe_file.relative_file_path
        )
    )
    # ... but with changed file content
    api_client.recipes.upload_file_content(api_recipe.id, api_file.id, b"Old file content")

    # 3. Load objects collection from API
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)

    # 4. And build a diff. Diff should contain only recipe's change
    diff = DiffBuilder().build(iac_collection, api_collection)

    # 5. Apply diff
    DiffApplier().apply(api_client, api_collection, diff)

    # 6. Check that recipe changed their id, ...
    recipe_list = api_client.recipes.list()
    assert len(recipe_list) == 1
    assert recipe_list[0].id != api_recipe.id
    assert recipe_list[0].name == test_recipe.name
    assert recipe_list[0].description == test_recipe.description
    assert recipe_list[0].files[0].relative_file_path == test_recipe_file.relative_file_path

    # ... and that file changed it's content
    assert api_client.recipes.get_file_content(
        recipe_list[0].id, recipe_list[0].files[0].id
    ) == test_recipe_file.content

    # 7. Cluster is created from new recipe
    cluster_list = api_client.clusters.list()
    assert len(cluster_list) == 1
    assert cluster_list[0].recipe == recipe_list[0]


def test_prober_updated_if_there_is_file_change(client: ModelBasedHttpClient, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    # 1. Create a local IaC objects collection with one prober
    iac_collection = ObjectsCollection(
        probers={
            test_prober.arcadia_path: test_prober,
        }
    )

    # 2. Create a prober in API, ...
    api_prober = api_client.probers.create(
        api.models.CreateProberRequest(
            manually_created=False,
            arcadia_path="probers/network/dns/prober.yaml",
            name="DNS",
            slug="dns",
            description="Simple DNS check",
            runner=BashProberRunner(
                command="./dns.sh",
            ),
        )
    )
    api_file = api_client.probers.add_file(
        api_prober.id, api.models.CreateProberFileRequest(
            relative_file_path=test_prober_file.relative_file_path,
        )
    )
    # ... with changed file content
    api_client.probers.upload_file_content(api_prober.id, api_file.id, b"#!/usr/bin/python")

    # 3. Load objects collection from API
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)

    # 4. And build a diff. Diff should contain only prober's change
    diff = DiffBuilder().build(iac_collection, api_collection)

    # 5. Apply diff
    DiffApplier().apply(api_client, api_collection, diff)

    # 6. Check that prober changed their id, ...
    probers_list = api_client.probers.list()
    assert len(probers_list) == 1
    assert probers_list[0].id != api_prober.id
    assert probers_list[0].name == test_prober.name
    assert probers_list[0].slug == test_prober.slug
    assert probers_list[0].description == test_prober.description
    assert probers_list[0].files[0].relative_file_path == test_prober_file.relative_file_path

    # ... and that file changed it's content
    assert api_client.probers.get_file_content(
        probers_list[0].id, probers_list[0].files[0].id
    ) == test_prober_file.content

    assert len(probers_list[0].configs) == 1


def test_diff_is_empty_after_apply(client: ModelBasedHttpClient, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    # 1. Create a local IaC objects collection with recipe, cluster and prober
    iac_collection = ObjectsCollection(
        recipes={
            test_recipe.arcadia_path: test_recipe,
        },
        clusters={
            test_cluster.arcadia_path: test_cluster,
        },
        probers={
            test_prober.arcadia_path: test_prober,
        },
    )

    # 2. Load empty collection from API
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)

    # 3. Build a diff and apply it
    diff = DiffBuilder().build(iac_collection, api_collection)
    DiffApplier().apply(api_client, api_collection, diff)

    # 4. Re-load collection from API
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)
    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.is_empty()


def test_change_only_prober_file_executable(client: ModelBasedHttpClient, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    # 1. Create a prober in API, the same as `test_prober`, but with files[0].is_executable = False
    api_prober = api_client.probers.create(
        api.models.CreateProberRequest(
            manually_created=False,
            arcadia_path=test_prober.arcadia_path,
            name=test_prober.name,
            slug=test_prober.slug,
            description=test_prober.description,
            runner=test_prober.runner,
        )
    )
    api_file = api_client.probers.add_file(
        api_prober.id, api.models.CreateProberFileRequest(
            relative_file_path=test_prober_file.relative_file_path,
            is_executable=False,
        )
    )
    api_client.probers.upload_file_content(api_prober.id, api_file.id, test_prober_file.content)

    # 2. Create IaC and API collections
    iac_collection = ObjectsCollection(
        probers={
            test_prober.arcadia_path: test_prober,
        },
    )
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)

    # 3. Build diff and check that prober is marked as changed
    diff = DiffBuilder().build(iac_collection, api_collection)
    assert len(diff.probers.changed) == 1

    # 4. Apply the diff!
    DiffApplier().apply(api_client, api_collection, diff)

    # 5. Check that API has a prober file with is_executable=True now
    assert api_client.probers.get(1).files[0].is_executable

    # 6. Re-download API collection and check that now diff is empty
    api_collection = ObjectsCollection.load_from_api_with_client(api_client)
    diff = DiffBuilder().build(iac_collection, api_collection)
    assert diff.is_empty()
