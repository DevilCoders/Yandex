import logging
from pathlib import Path
from typing import List, TypeVar, Generic, Dict, Tuple, Optional

from pydantic import BaseModel
from rich.console import Console
from rich.prompt import Confirm

import tools.synchronize.data.models as data_models
from api.client import MrProberApiClient
from tools.synchronize.formatter import PrettyFormatWithLongStringsTruncated
from tools.synchronize.data.collection import ObjectsCollection

__all__ = ["SpecificTypeDiff", "Diff", "DiffBuilder", "DiffApplier", "ApplyCanceled"]

Model = TypeVar("Model", data_models.Cluster, data_models.Recipe, data_models.Prober)


class SpecificTypeDiff(BaseModel, Generic[Model]):
    added: List[Model] = []
    deleted: List[Model] = []
    changed: List[Tuple[Model, Model]] = []

    def is_empty(self) -> bool:
        return not any((self.added, self.changed, self.deleted))


class Diff(BaseModel):
    clusters: SpecificTypeDiff[data_models.Cluster]
    recipes: SpecificTypeDiff[data_models.Recipe]
    probers: SpecificTypeDiff[data_models.Prober]

    def is_empty(self) -> bool:
        for specific_diff in (self.clusters, self.recipes, self.probers):
            if not specific_diff.is_empty():
                return False
        return True


class DiffBuilder:
    def __init__(self, include: Optional[List[str]] = None):
        self._included_paths = None
        if include is not None:
            self._included_paths = [Path(p) for p in include]

    def build(self, iac_collection: ObjectsCollection, api_collection: ObjectsCollection) -> Diff:
        self._compose_untracked_variables(iac_collection, api_collection)
        return Diff(
            clusters=self._build_specific_type_diff(iac_collection.clusters, api_collection.clusters),
            recipes=self._build_specific_type_diff(iac_collection.recipes, api_collection.recipes),
            probers=self._build_specific_type_diff(iac_collection.probers, api_collection.probers),
        )

    def _build_specific_type_diff(self, iac: Dict[str, Model], api: Dict[str, Model]) -> SpecificTypeDiff[Model]:
        result = SpecificTypeDiff()

        for arcadia_path, obj in iac.items():
            if not self._is_included(arcadia_path):
                continue

            if arcadia_path not in api:
                result.added.append(obj)
            elif not obj.equals(api[arcadia_path]):
                result.changed.append((api[arcadia_path], obj))

        for deleted_arcadia_path in set(api.keys()) - set(iac.keys()):
            if not self._is_included(deleted_arcadia_path):
                continue

            result.deleted.append(api[deleted_arcadia_path])

        return result

    @staticmethod
    def _compose_untracked_variables(iac: ObjectsCollection, api: ObjectsCollection):
        # Diff don't count untracked variable
        for arcadia_path in iac.clusters.keys() & api.clusters.keys():
            api_cluster = api.clusters[arcadia_path]
            iac_cluster = iac.clusters[arcadia_path]

            # first: in iac update all untracked variables from api, because these variables managed from API only
            for untracked in iac_cluster.untracked_variables:
                if untracked in api_cluster.variables:
                    iac_cluster.variables[untracked] = api_cluster.variables[untracked]

            # second: add list of untracked variables into api, because them exists in IaC only
            api_cluster.untracked_variables = iac_cluster.untracked_variables

    def _is_included(self, path_str: str) -> bool:
        if self._included_paths is None:
            return True

        path = Path(path_str)
        return any(
            included_path in path.parents for included_path in self._included_paths
        )


class DiffApplier:
    def __init__(self, console: Optional[Console] = None, need_confirmation: bool = False):
        if console is None:
            console = Console()

        self._console = console
        self._need_confirmation = need_confirmation
        self._formatter = PrettyFormatWithLongStringsTruncated(strings_max_length=60, repr_strings=True)

    def apply(self, client: MrProberApiClient, api_collection: ObjectsCollection, diff: Diff):
        # NOTE: Order of following statements is very important!

        # First, we delete probers and clusters marked as deleted.
        self._delete_specific_type_objects(client, diff.probers, api_collection, api_collection.probers)
        # Cluster deleting is not supported now, exception will be thrown if any of cluster was marked as deleted.
        # Let's assume that is OK for now, we will support cluster deleting later.
        self._delete_specific_type_objects(client, diff.clusters, api_collection, api_collection.clusters)

        # Second, we create all objects: recipes, clusters build on recipes, and probers which may have configs
        # for specific cluster
        self._add_specific_type_objects(client, diff.recipes, api_collection, api_collection.recipes)
        self._add_specific_type_objects(client, diff.clusters, api_collection, api_collection.clusters)
        self._add_specific_type_objects(client, diff.probers, api_collection, api_collection.probers)

        # Then, we update existing entities
        self._change_specific_type_objects(client, diff.recipes, api_collection, api_collection.recipes)
        self._change_specific_type_objects(client, diff.clusters, api_collection, api_collection.clusters)
        self._change_specific_type_objects(client, diff.probers, api_collection, api_collection.probers)

        # Last, we delete recipes, because some clusters probably moved from this recipe one step above
        self._delete_specific_type_objects(client, diff.recipes, api_collection, api_collection.recipes)

    def _delete_specific_type_objects(
        self, client: MrProberApiClient, this_type_diff: SpecificTypeDiff[Model],
        collection: ObjectsCollection, this_type_collection: Dict[str, Model],
    ):
        for deleted_object in this_type_diff.deleted:
            if self._need_confirmation:
                self._console.print(
                    f":point_right: Going to [red3]delete[/red3] {self._formatter(deleted_object)}",
                    new_line_start=True
                )
                if not Confirm.ask("Delete?", console=self._console, default=False):
                    raise ApplyCanceled()

            deleted_object.delete_in_api(client, collection)
            del this_type_collection[deleted_object.arcadia_path]

            logging.info(f"Deleted [red3]{deleted_object.arcadia_path}[red3] from API")

    def _add_specific_type_objects(
        self, client: MrProberApiClient, this_type_diff: SpecificTypeDiff[Model],
        collection: ObjectsCollection, this_type_collection: Dict[str, Model],
    ):
        for added_object in this_type_diff.added:
            if self._need_confirmation:
                self._console.print(
                    f":point_right: Going to [green3]create[/green3] {self._formatter(added_object)}",
                    new_line_start=True
                )
                if not Confirm.ask("Create?", console=self._console, default=False):
                    raise ApplyCanceled()

            created = added_object.create_in_api(client, collection)
            this_type_collection[added_object.arcadia_path] = created

            logging.info(f"Created [green3]{added_object.arcadia_path}[/green3] in API")

    def _change_specific_type_objects(
        self, client: MrProberApiClient, this_type_diff: SpecificTypeDiff[Model],
        collection: ObjectsCollection, this_type_collection: Dict[str, Model],
    ):
        for changed_object, new_object_value in this_type_diff.changed:
            if self._need_confirmation:
                self._console.print(
                    f":point_right: Going to [yellow3]update[/yellow3] {self._formatter(changed_object)}",
                    new_line_start=True
                )
                if not Confirm.ask("Update?", console=self._console, default=False):
                    raise ApplyCanceled()

            updated = changed_object.update_in_api(client, collection, new_object_value)
            this_type_collection[changed_object.arcadia_path] = updated

            logging.info(f"Updated [yellow3]{changed_object.arcadia_path}[/yellow3] in API")


class ApplyCanceled(Exception):
    pass
