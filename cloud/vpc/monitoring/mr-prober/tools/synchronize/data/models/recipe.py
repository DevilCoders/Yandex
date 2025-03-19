import pathlib
from typing import Optional, List, Iterable

from pydantic import BaseModel

import api.models
import tools.synchronize.iac.models as iac_models
from api.client import MrProberApiClient
from tools.synchronize.equalable import EqualableMixin
from tools.synchronize.models import FileCollection

from tools.synchronize.data.models.utils import are_file_contents_changed, save_files_to_disk

__all__ = ["Recipe", "RecipeFile"]


class RecipeFile(BaseModel, EqualableMixin):
    id: Optional[int]
    relative_file_path: str
    content: bytes

    class Config:
        comparator_ignores_keys = {"id"}

    @classmethod
    def load_from_iac(cls, file_collection: FileCollection, base_path: pathlib.Path) -> Iterable["RecipeFile"]:
        for relative_file_path, filename in file_collection.get_files(base_path):
            yield cls(
                id=None,
                relative_file_path=relative_file_path.as_posix(),
                content=filename.read_bytes(),
            )

    @classmethod
    def load_from_api_model(
        cls, client: MrProberApiClient, recipe: api.models.ClusterRecipe,
        file: api.models.ClusterRecipeFile
    ) -> "RecipeFile":
        return cls(
            id=file.id,
            relative_file_path=file.relative_file_path,
            content=client.recipes.get_file_content(recipe.id, file.id),
        )


class Recipe(BaseModel, EqualableMixin):
    id: Optional[int]
    arcadia_path: str
    name: str
    description: str
    files: List[RecipeFile]

    class Config:
        comparator_ignores_keys = {"id": ..., "files": {"__all__": RecipeFile.Config.comparator_ignores_keys}}

    @classmethod
    def load_from_iac(cls, arcadia_path: pathlib.Path) -> "Recipe":
        recipe = iac_models.Recipe.load_from_file(arcadia_path)
        result = cls(
            id=None,
            arcadia_path=arcadia_path.as_posix(),
            name=recipe.name,
            description=recipe.description,
            files=[],
        )
        for file_collection in recipe.files:
            result.files.extend(RecipeFile.load_from_iac(file_collection, arcadia_path.parent))

        return result

    @classmethod
    def load_from_api(
        cls, client: MrProberApiClient, recipe_id: int, collection: Optional["ObjectsCollection"] = None
    ) -> "Recipe":
        recipe = client.recipes.get(recipe_id)
        return cls.load_from_api_model(client, recipe, collection)

    @classmethod
    def load_from_api_model(
        cls, client: MrProberApiClient, recipe: api.models.ClusterRecipe,
        collection: Optional["ObjectsCollection"] = None
    ) -> "Recipe":
        if collection is not None and recipe.arcadia_path in collection.recipes:
            return collection.recipes[recipe.arcadia_path]

        files = [RecipeFile.load_from_api_model(client, recipe, file) for file in recipe.files]

        return cls(
            id=recipe.id,
            arcadia_path=recipe.arcadia_path,
            name=recipe.name,
            description=recipe.description,
            files=files,
        )

    def delete_in_api(self, client: MrProberApiClient, collection: "ObjectsCollection"):
        assert self.id is not None

        client.recipes.delete(self.id)

    def create_in_api(self, client: MrProberApiClient, collection: "ObjectsCollection") -> "Recipe":
        assert self.id is None

        created_recipe = client.recipes.create(
            api.models.CreateClusterRecipeRequest(
                manually_created=False,
                arcadia_path=self.arcadia_path,
                name=self.name,
                description=self.description,
            )
        )

        for file in self.files:
            assert file.id is None

            created_file = client.recipes.add_file(
                created_recipe.id, api.models.CreateClusterRecipeFileRequest(
                    relative_file_path=file.relative_file_path,
                )
            )
            client.recipes.upload_file_content(created_recipe.id, created_file.id, file.content)

        return Recipe.load_from_api_model(client, created_recipe)

    def update_in_api(
        self, client: MrProberApiClient, collection: "ObjectsCollection", new_recipe: "Recipe"
    ) -> "Recipe":
        assert self.id is not None

        if are_file_contents_changed(self.files, new_recipe.files):
            recipe_copy = new_recipe.create_in_api(client, collection)
            self._migrate_clusters_to_new_recipe(client, self.id, recipe_copy.id)
            self.delete_in_api(client, collection)
            return recipe_copy

        recipe_from_api = client.recipes.update(
            self.id,
            api.models.UpdateClusterRecipeRequest(
                manually_created=False,
                arcadia_path=new_recipe.arcadia_path,
                name=new_recipe.name,
                description=new_recipe.description,
            )
        )

        return Recipe.load_from_api_model(client, recipe_from_api)

    @staticmethod
    def _migrate_clusters_to_new_recipe(client: MrProberApiClient, recipe_id: int, new_recipe_id: int):
        for cluster in client.clusters.list():
            if cluster.recipe.id == recipe_id:
                client.clusters.update(
                    cluster.id, api.models.UpdateClusterRequest(
                        recipe_id=new_recipe_id,

                        manually_created=cluster.manually_created,
                        arcadia_path=cluster.arcadia_path,
                        name=cluster.name,
                        slug=cluster.slug,
                        variables={variable.name: variable.value for variable in cluster.variables},
                    )
                )

    def save_to_iac(self, path: pathlib.Path):
        path.parent.mkdir(parents=True, exist_ok=True)

        if self.files:
            save_files_to_disk(self.files, path.parent / "files")

        recipe = iac_models.Recipe(
            name=self.name,
            description=self.description,
            files=[FileCollection(directory="files/")] if self.files else [],
        )

        recipe.save_to_file(path)
