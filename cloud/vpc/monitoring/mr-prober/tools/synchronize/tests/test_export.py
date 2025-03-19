import pathlib
import tempfile

from rich.console import Console

import api.models
import tools.synchronize.iac.models as iac_models
from api.client import ModelBasedHttpClient, MrProberApiClient
from tools.synchronize import cli
from tools.synchronize.data.collection import ObjectsCollection
from tools.synchronize.diff import DiffBuilder
from tools.synchronize.export import export_objects, ExportableObjectType, filter_objects_by_search_request

console = Console()


def test_export_empty_set_of_objects(client: ModelBasedHttpClient):
    api_client = MrProberApiClient(underlying_client=client)

    objects = export_objects(
        console, ExportableObjectType.RECIPES, api_client, set(),
        unset_manually_created_flag=False, base_path=pathlib.Path(".")
    )

    assert len(objects) == 0


def test_export_objects_single_recipe(client: ModelBasedHttpClient, mocked_confirmation):
    api_client = MrProberApiClient(underlying_client=client)
    api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="recipes/test/recipe.yaml",
            name="test",
            description="Test",
        )
    )

    with tempfile.TemporaryDirectory() as directory:
        objects = export_objects(
            console, ExportableObjectType.RECIPES, api_client, set(),
            unset_manually_created_flag=False, base_path=pathlib.Path(directory)
        )

        assert len(objects) == 1
        assert objects[0].name == "test"
        assert objects[0].arcadia_path == "recipes/test/recipe.yaml"

        target_file = pathlib.Path(directory) / "recipes/test/recipe.yaml"
        assert target_file.exists()

        recipe = iac_models.Recipe.load_from_file(target_file)
        assert recipe.name == "test"
        assert recipe.description == "Test"
        assert len(recipe.files) == 0


def test_export_objects_single_recipe_with_file(client: ModelBasedHttpClient, mocked_confirmation):
    api_client = MrProberApiClient(underlying_client=client)
    api_recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="recipes/test/recipe.yaml",
            name="test",
            description="Test",
        )
    )
    api_file = api_client.recipes.add_file(
        api_recipe.id, api.models.CreateClusterRecipeFileRequest(
            relative_file_path="test.tf",
        )
    )
    api_client.recipes.upload_file_content(api_recipe.id, api_file.id, b"resource {}")

    with tempfile.TemporaryDirectory() as directory:
        export_objects(
            console, ExportableObjectType.RECIPES, api_client, set(),
            unset_manually_created_flag=False, base_path=pathlib.Path(directory)
        )

        target_file = pathlib.Path(directory) / "recipes/test/recipe.yaml"
        recipe = iac_models.Recipe.load_from_file(target_file)
        assert len(recipe.files) == 1
        assert recipe.files[0].directory == "files/"

        recipe_file = target_file.parent / "files/test.tf"
        assert recipe_file.exists()
        assert recipe_file.read_bytes() == b"resource {}"


def test_export_objects_ignores_not_manually_created_objects(client: ModelBasedHttpClient):
    api_client = MrProberApiClient(underlying_client=client)
    api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=False,
            arcadia_path="recipes/test/recipe.yaml",
            name="test",
            description="Test",
        )
    )

    objects = export_objects(
        console, ExportableObjectType.RECIPES, api_client, set(),
        unset_manually_created_flag=False, base_path=pathlib.Path(".")
    )

    assert len(objects) == 0


def test_filter_objects_works_with_recipes_download_from_api(client: ModelBasedHttpClient):
    api_client = MrProberApiClient(underlying_client=client)
    recipe1 = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="recipes/test1/recipe.yaml",
            name="test",
            description="Test",
        )
    )
    recipe2 = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="recipes/test2/recipe.yaml",
            name="second_recipe",
            description="Test",
        )
    )

    recipes = api_client.recipes.list()

    result = list(filter_objects_by_search_request(recipes, set()))
    assert len(result) == 2
    assert recipe1 in result and recipe2 in result

    result = list(filter_objects_by_search_request(recipes, {"second_recipe"}))
    assert len(result) == 1
    assert result[0] == recipe2

    result = list(filter_objects_by_search_request(recipes, {str(recipe1.id)}))
    assert len(result) == 1
    assert result[0] == recipe1

    result = list(filter_objects_by_search_request(recipes, {r".*/test2/.*"}))
    assert len(result) == 1
    assert result[0] == recipe2

    result = list(filter_objects_by_search_request(recipes, {r".*/test\d/.*"}))
    assert len(result) == 2

    result = list(filter_objects_by_search_request(recipes, {str(recipe1.id), "second_recipe"}))
    assert len(result) == 2


def test_filter_objects_works_in_difficult_cases():
    recipe = api.models.ClusterRecipe(
        id=1,
        manually_created=True,
        arcadia_path="recipes/test/recipe.yaml",
        name="test",
        description="Test",
        files=[],
    )
    clusters = []
    for i in range(1, 30):
        clusters.append(
            api.models.Cluster(
                id=i,
                manually_created=True,
                arcadia_path=f"clusters/{i}/cluster.yaml",
                recipe=recipe,
                name=f"Test {i}",
                slug=f"test-{i}",
                variables=[api.models.ClusterVariable(id=i, name="index", value=i)],
            )
        )

    result = list(filter_objects_by_search_request(clusters, {"1"}))
    assert len(result) == 1

    result = list(filter_objects_by_search_request(clusters, {r"clusters/1\d/.+", r"clusters/2\d/.+"}))
    assert len(result) == 20

    result = list(filter_objects_by_search_request(clusters, {r".+/15/.+", "Test 15", "15", "unknown-cluster"}))
    assert len(result) == 1


def test_diff_is_empty_after_export(client: ModelBasedHttpClient, mocked_s3, mocked_confirmation):
    """
    Large test for full workflow:
    - create an empty IaC configuration
    - create manually-created objects in API
    - export manually-created objects from API into IaC configuration
    - check that diff is empty
    """
    api_client = MrProberApiClient(underlying_client=client)

    # 1. Create recipe, cluster and prober in API. Mark them as manually-created
    recipe = api_client.recipes.create(
        api.models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="recipes/test/recipe.yaml",
            name="test",
            description="Test",
        )
    )
    recipe_file = api_client.recipes.add_file(
        recipe.id, api.models.CreateClusterRecipeFileRequest(
            relative_file_path="test.tf",
        )
    )
    api_client.recipes.upload_file_content(recipe.id, recipe_file.id, b"resource {}")

    # Re-download the recipe, because not it has a file
    recipe = api_client.recipes.get(recipe.id)

    cluster = api_client.clusters.create(
        api.models.CreateClusterRequest(
            manually_created=True,
            arcadia_path="clusters/test/cluster.yaml",
            name="Test",
            slug="test",
            recipe_id=recipe.id,
            variables={
                "key": "value",
            }
        )
    )

    prober = api_client.probers.create(
        api.models.CreateProberRequest(
            manually_created=True,
            arcadia_path="probers/test/prober.yaml",
            name="Test",
            slug="test",
            description="Test",
            runner=api.models.BashProberRunner(command="./test"),
        )
    )

    # 2. Create local IaC configuration
    config = iac_models.Config(
        environments={
            "test": iac_models.Environment(
                endpoint="http://test",
                clusters=["clusters/*/cluster.yaml"],
                probers=["probers/**/prober.yaml"],
            )
        }
    )

    # ... and save it to temporary file
    with tempfile.TemporaryDirectory() as directory:
        directory = pathlib.Path(directory)
        config_file = directory / "config.yaml"
        config.save_to_file(config_file)

        # 3. Load an objects collection from IaC, load another object collection from API, build diff
        environment = cli.get_environment(config_file.as_posix(), "test")
        iac_collection = ObjectsCollection.load_from_iac(environment, directory)
        # Check that IAC collection is empty on the start
        assert not iac_collection.clusters and not iac_collection.recipes and not iac_collection.probers
        api_collection = ObjectsCollection.load_from_api_with_client(api_client)
        diff = DiffBuilder().build(iac_collection, api_collection)

        # 4. Diff is empty, because IaC collection contains nothing, and all API objects has manually_created = True
        assert diff.is_empty()

        # 5. Export all objects from API: recipes first, cluster and probers later
        for exported_type in (
                ExportableObjectType.RECIPES, ExportableObjectType.CLUSTERS, ExportableObjectType.PROBERS):
            exported_objects = export_objects(
                console, exported_type, api_client, set(), unset_manually_created_flag=True, base_path=directory,
            )
            # Check that only one object has been exported in each category
            assert len(exported_objects) == 1

        # 6. Check that recipe and cluster are not marked as manually_created in API
        assert not api_client.recipes.get(recipe.id).manually_created
        assert not api_client.clusters.get(cluster.id).manually_created
        # 7. Check that file with prober configuration has been created
        assert (directory / "probers/test/prober.yaml").exists()

        # 8. Reload objects collection from IaC after export
        iac_collection = ObjectsCollection.load_from_iac(environment, directory)
        # Check that IaC collection is not empty anymore after successful export
        assert len(iac_collection.clusters) == 1
        assert len(iac_collection.probers) == 1
        assert len(iac_collection.recipes) == 1
        # 9. Reload objects collection from API
        api_collection = ObjectsCollection.load_from_api_with_client(api_client)
        diff = DiffBuilder().build(iac_collection, api_collection)

        # 10. Diff is empty after export!
        assert diff.is_empty()
