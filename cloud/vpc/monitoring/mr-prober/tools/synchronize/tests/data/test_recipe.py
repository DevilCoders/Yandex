import pathlib

import tools.synchronize.data.models as data_models


def test_recipe_load_from_iac_with_excluded_files(testdata):
    recipe = data_models.Recipe.load_from_iac(pathlib.Path("recipes/meeseeks/recipe.yaml"))

    assert recipe.name == "meeseeks"
    assert recipe.arcadia_path == "recipes/meeseeks/recipe.yaml"

    assert set(recipe_file.relative_file_path for recipe_file in recipe.files) == {
        "conductor.tf",
        "instances.tf",
        # "variables.tf" not loaded due to exclude directive
    }


def test_recipe_load_from_iac_recursive(testdata):
    recipe = data_models.Recipe.load_from_iac(pathlib.Path("recipes/modules/recipe.yaml"))

    assert recipe.name == "modules"
    assert recipe.arcadia_path == "recipes/modules/recipe.yaml"

    assert set(recipe_file.relative_file_path for recipe_file in recipe.files) == {
        "module/main.tf",
    }
