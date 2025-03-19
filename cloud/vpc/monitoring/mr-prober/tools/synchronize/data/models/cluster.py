import pathlib
from typing import Optional, Dict, List

from pydantic import BaseModel

import api.models
import tools.synchronize.iac.models as iac_models
from api.client import MrProberApiClient
from tools.synchronize.equalable import EqualableMixin
from tools.synchronize.models import VariableType
from .recipe import Recipe

__all__ = ["Cluster"]


class Cluster(BaseModel, EqualableMixin):
    id: Optional[int] = None
    arcadia_path: str
    name: str
    slug: str
    recipe: Recipe
    variables: Dict[str, VariableType]
    untracked_variables: List[str] = []
    deploy_policy: Optional[api.models.ClusterDeployPolicy] = None

    class Config(BaseModel.Config):
        comparator_ignores_keys = {"id": ..., "recipe": set(Recipe.__fields__.keys()) - {"arcadia_path"}}

    @classmethod
    def load_from_iac(cls, arcadia_path: pathlib.Path) -> "Cluster":
        cluster = iac_models.Cluster.load_from_file(arcadia_path)
        recipe = Recipe.load_from_iac(pathlib.Path(cluster.recipe))

        return cls(
            id=None,
            arcadia_path=arcadia_path.as_posix(),
            name=cluster.name,
            slug=cluster.slug,
            recipe=recipe,
            variables=cluster.variables,
            untracked_variables=cluster.untracked_variables,
            deploy_policy=cluster.deploy_policy,
        )

    @classmethod
    def load_from_api(
        cls, client: MrProberApiClient, cluster_id: int, collection: Optional["ObjectsCollection"] = None
    ) -> "Cluster":
        cluster = client.clusters.get(cluster_id)
        return cls.load_from_api_model(client, cluster, collection)

    @classmethod
    def load_from_api_model(
        cls, client: MrProberApiClient, cluster: api.models.Cluster, collection: Optional["ObjectsCollection"] = None
    ) -> "Cluster":
        if collection is not None and cluster.arcadia_path in collection.clusters:
            return collection.clusters[cluster.arcadia_path]

        return cls(
            id=cluster.id,
            arcadia_path=cluster.arcadia_path,
            name=cluster.name,
            slug=cluster.slug,
            recipe=Recipe.load_from_api_model(client, cluster.recipe, collection),
            variables={v.name: v.value for v in cluster.variables},
            untracked_variables=[],
            deploy_policy=cluster.deploy_policy,
        )

    def delete_in_api(self, client: MrProberApiClient, collection: "ObjectsCollection"):
        assert self.id is not None

        client.clusters.delete(self.id)

    def create_in_api(self, client: MrProberApiClient, collection: "ObjectsCollection") -> "Cluster":
        assert self.id is None
        assert self.recipe.arcadia_path in collection.recipes, \
            f"The recipe ({self.recipe.arcadia_path}) should be created in API before the cluster ({self.arcadia_path})"

        created_cluster = client.clusters.create(
            api.models.CreateClusterRequest(
                recipe_id=collection.recipes[self.recipe.arcadia_path].id,
                name=self.name,
                slug=self.slug,
                manually_created=False,
                arcadia_path=self.arcadia_path,
                variables=self.variables,
                deploy_policy=self.deploy_policy,
            )
        )

        return Cluster.load_from_api_model(client, created_cluster)

    def update_in_api(
        self, client: MrProberApiClient, collection: "ObjectsCollection",
        new_cluster: "Cluster"
    ) -> "Cluster":
        assert self.id is not None
        assert new_cluster.recipe.arcadia_path in collection.recipes, \
            f"The recipe ({new_cluster.recipe.arcadia_path}) should be created in API before the cluster ({self.arcadia_path})"

        updated_cluster = client.clusters.update(
            self.id,
            api.models.UpdateClusterRequest(
                manually_created=False,
                arcadia_path=new_cluster.arcadia_path,
                recipe_id=collection.recipes[new_cluster.recipe.arcadia_path].id,
                name=new_cluster.name,
                slug=new_cluster.slug,
                variables=new_cluster.variables,
                deploy_policy=new_cluster.deploy_policy,
            )
        )

        return Cluster.load_from_api_model(client, updated_cluster)

    def save_to_iac(self, path: pathlib.Path):
        path.parent.mkdir(parents=True, exist_ok=True)

        cluster = iac_models.Cluster(
            name=self.name,
            slug=self.slug,
            recipe=self.recipe.arcadia_path,
            variables=self.variables,
            deploy_policy=self.deploy_policy,
        )
        cluster.save_to_file(path)
