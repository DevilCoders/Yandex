import pathlib

import api.models
from api.client import MrProberApiClient
from tools.synchronize.data.collection import ObjectsCollection
from tools.synchronize.iac.models import Environment


def test_collection_load_from_iac(testdata):
    environment = Environment(
        endpoint="https://api.prober.cloud.yandex.net",
        clusters=[
            "clusters/prod/*/cluster.yaml",
        ],
        probers=[
            "probers/network/**/prober.yaml",
        ],
    )
    collection = ObjectsCollection.load_from_iac(environment, pathlib.Path("."))

    assert len(collection.clusters) == 1
    prod_cluster = collection.clusters["clusters/prod/meeseeks/cluster.yaml"]

    assert len(collection.probers) == 2
    prober = collection.probers["probers/network/dns/dns-resolve-yandex-host/prober.yaml"]

    # Check that there is a config only for prod cluster in the collection, not for preprod
    assert len(prober.configs) == 1
    assert prober.configs[0].cluster == prod_cluster

    prober = collection.probers["probers/network/check-http-v4/prober.yaml"]

    # Check that there is a config only for prod cluster in the collection, not for preprod
    assert len(prober.configs) == 2
    assert prober.configs[1].cluster == prod_cluster
    assert prober.configs[0].variables == {"interface": "eth0", "connect_timeout": 3}
    assert type(prober.configs[0].variables["connect_timeout"]) is int
    assert prober.configs[1].variables == {"url": "${matrix.url}"}
    assert prober.configs[1].matrix_variables == {"url": ["https://yandex.ru", "https://google.com"]}


def test_empty_collection_load_from_api(client, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)
    collection = ObjectsCollection.load_from_api_with_client(api_client)

    assert len(collection.clusters) == 0
    assert len(collection.recipes) == 0
    assert len(collection.probers) == 0


def test_collection_load_from_api_ignore_manually_created_objects(client, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            name="Manually Created",
            arcadia_path="",
            description="Just a manually created recipe",
        )
    )

    api_client.clusters.create(
        api.models.CreateClusterRequest(
            recipe_id=recipe.id,
            manually_created=True,
            arcadia_path="",
            name="Manually Created",
            slug="manually-created",
            description="Just a manually created cluster",
            variables={},
        )
    )

    collection = ObjectsCollection.load_from_api_with_client(api_client)

    assert len(collection.recipes) == 0
    assert len(collection.clusters) == 0


def test_collection_load_from_api_in_simple_case(client, mocked_s3):
    api_client = MrProberApiClient(underlying_client=client)

    recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=False,
            name="Meeseeks",
            arcadia_path="recipes/meeseeks/recipe.yaml",
            description="Meeseeks recipe",
        )
    )

    api_client.clusters.create(
        api.models.CreateClusterRequest(
            recipe_id=recipe.id,
            manually_created=False,
            arcadia_path="clusters/prod/meeseeks/cluster.yaml",
            name="Meeseeks",
            slug="meeseeks",
            variables={
                "cluster_id": 1,
            },
        )
    )

    collection = ObjectsCollection.load_from_api_with_client(api_client)

    assert len(collection.recipes) == 1
    assert len(collection.clusters) == 1

    cluster = collection.clusters["clusters/prod/meeseeks/cluster.yaml"]
    assert cluster.name == "Meeseeks"
    assert cluster.slug == "meeseeks"
    assert cluster.variables == {
        "cluster_id": 1,
    }
