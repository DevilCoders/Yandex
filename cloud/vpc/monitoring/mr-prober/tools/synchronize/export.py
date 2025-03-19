import enum
import logging
import os
import pathlib
import re
import sys
from typing import List, Set, Iterable, Type, TypeVar, Dict, Callable

from pydantic import BaseModel
from rich.console import Console
from rich.pretty import Pretty
from rich.prompt import Confirm
from rich.table import Table

import api.models as api_models
import tools.synchronize.data.models as data_models
from tools.common import StopCliProcess
from api.client import MrProberApiClient

__all__ = ["ExportableObjectType", "export_objects", "filter_objects_by_search_request"]
Model = TypeVar("Model", bound=BaseModel)


class ExportableObjectType(str, enum.Enum):
    RECIPES = "recipes"
    CLUSTERS = "clusters"
    PROBERS = "probers"


data_model_by_object_type: Dict[ExportableObjectType, Type[Model]] = {
    ExportableObjectType.RECIPES: data_models.Recipe,
    ExportableObjectType.CLUSTERS: data_models.Cluster,
    ExportableObjectType.PROBERS: data_models.Prober,
}

object_list_by_object_type: Dict[ExportableObjectType, Callable[[MrProberApiClient], List[Model]]] = {
    ExportableObjectType.RECIPES: lambda client: client.recipes.list(),
    ExportableObjectType.CLUSTERS: lambda client: client.clusters.list(),
    ExportableObjectType.PROBERS: lambda client: client.probers.list(),
}


def mark_recipe_as_iac_created(client: MrProberApiClient, recipe: api_models.ClusterRecipe):
    client.recipes.update(
        recipe.id, api_models.UpdateClusterRecipeRequest(
            manually_created=False,
            arcadia_path=recipe.arcadia_path,
            name=recipe.name,
            description=recipe.description,
        )
    )


def mark_cluster_as_iac_created(client: MrProberApiClient, cluster: api_models.Cluster):
    client.clusters.update(
        cluster.id, api_models.UpdateClusterRequest(
            manually_created=False,
            arcadia_path=cluster.arcadia_path,

            recipe_id=cluster.recipe.id,
            name=cluster.name,
            slug=cluster.slug,
            variables={variable.name: variable.value for variable in cluster.variables},
        )
    )


def mark_prober_as_iac_created(client: MrProberApiClient, prober: api_models.Prober):
    client.probers.update(
        prober.id, api_models.UpdateProberRequest(
            manually_created=False,
            arcadia_path=prober.arcadia_path,

            name=prober.name,
            slug=prober.slug,
            description=prober.description,
            runner=prober.runner,
        )
    )


mark_function_by_object_type: Dict[ExportableObjectType, Callable[[MrProberApiClient, Model], None]] = {
    ExportableObjectType.RECIPES: mark_recipe_as_iac_created,
    ExportableObjectType.CLUSTERS: mark_cluster_as_iac_created,
    ExportableObjectType.PROBERS: mark_prober_as_iac_created,
}


def clean_up_prober_from_manually_created_configs(prober: api_models.Prober) -> api_models.Prober:
    return prober.copy(update={"configs": [config for config in prober.configs if not config.manually_created]})


def forbid_cluster_created_from_manually_created_recipe(cluster: api_models.Cluster) -> api_models.Cluster:
    if cluster.recipe.manually_created:
        logging.error(
            f"Can not export cluster {cluster.name!r}, "
            f"because it was created from manually-created recipe. Export recipe first:\n"
            f"'{sys.argv[0]} export recipes {cluster.recipe.id} -c ... -e ... --unset-manually-created-flag'"
        )
        raise StopCliProcess(f"Can not export cluster {cluster.name!r}, export recipe {cluster.recipe.name!r} first.")
    return cluster


preprocess_function_by_object_type: Dict[ExportableObjectType, Callable[[Model], Model]] = {
    ExportableObjectType.RECIPES: lambda recipe: recipe,
    ExportableObjectType.CLUSTERS: forbid_cluster_created_from_manually_created_recipe,
    ExportableObjectType.PROBERS: clean_up_prober_from_manually_created_configs,
}


def filter_objects_by_search_request(objects_list: List, search_terms: Set[str]) -> Iterable:
    for obj in objects_list:
        if not obj.manually_created:
            continue

        satisfy_search_terms = (
                str(obj.id) in search_terms or
                getattr(obj, "name") in search_terms or
                any(re.fullmatch(term, obj.arcadia_path) for term in search_terms)
        )
        if not search_terms or satisfy_search_terms:
            if not obj.arcadia_path:
                logging.warning(f"Can't export {obj!r}, because [bold red3]it's arcadia_path is empty[/bold red3]")
            else:
                yield obj


def confirm_file_overwriting_if_needed(target_file):
    if target_file.exists():
        logging.warning(f"File {target_file.as_posix()!r} already exists. Do you want to overwrite it?")
        if not Confirm.ask("Overwrite?", default=False):
            raise StopCliProcess("Stopped by user")


def export_object(client: MrProberApiClient, obj: api_models.ClusterRecipe, data_model_type: Type[Model]):
    target_file = pathlib.Path(obj.arcadia_path)
    confirm_file_overwriting_if_needed(target_file)

    data_object = data_model_type.load_from_api_model(client, obj)
    data_object.save_to_iac(target_file)


def export_objects(
    console: Console, object_type: ExportableObjectType,
    client: MrProberApiClient, search_terms: Set[str],
    unset_manually_created_flag: bool, base_path: pathlib.Path,
) -> List[Model]:
    logging.info(f"Looking for {object_type} by search request {list(search_terms)!r} on {client}")

    object_list = object_list_by_object_type[object_type](client)
    api_objects = list(filter_objects_by_search_request(object_list, search_terms))

    if not api_objects:
        logging.error(f"Specified {object_type} not found on {client}")
        return api_objects

    preprocess_function = preprocess_function_by_object_type[object_type]
    api_objects = [preprocess_function(obj) for obj in api_objects]

    table = Table(
        "Id", "Name (path)", "Description", "Details",
        title=f"Found {len(api_objects)} {object_type}", title_style="bright_white bold", title_justify="left",
        leading=1,
    )
    for obj in api_objects:
        table.add_row(
            str(obj.id),
            f"{obj.name} ({obj.arcadia_path})",
            getattr(obj, "description", ""),
            Pretty(obj.dict(), overflow="ellipsis"),
        )
    console.print(table, new_line_start=True)

    if not Confirm.ask(f"Export {len(api_objects)} {object_type}?", default=False):
        raise StopCliProcess("Stopped by user")

    cwd = os.getcwd()
    try:
        os.chdir(base_path.as_posix())

        for obj in api_objects:
            short_object_repr = f"{obj.__class__.__name__}(id={obj.id}, name={obj.name!r})"

            logging.info(f"Exporting {short_object_repr} into {obj.arcadia_path!r}...")
            export_object(client, obj, data_model_type=data_model_by_object_type[object_type])

            if unset_manually_created_flag:
                logging.info(
                    f"Marking {short_object_repr} as IaC created in Mr. Prober API "
                    f"via setting manually_created=False..."
                )
                mark_function_by_object_type[object_type](client, obj)
    finally:
        os.chdir(cwd)

    return api_objects
