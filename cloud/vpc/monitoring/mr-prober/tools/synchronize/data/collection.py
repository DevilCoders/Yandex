import glob
import logging
import os
import pathlib
from typing import Dict

from pydantic import BaseModel

import tools.synchronize.data.models as data_models
from api.client import MrProberApiClient
from tools.common import StopCliProcess, ExitCode
from tools.synchronize.iac import models as iac_models


class ObjectsCollection(BaseModel):
    clusters: Dict[str, data_models.Cluster] = {}
    recipes: Dict[str, data_models.Recipe] = {}
    probers: Dict[str, data_models.Prober] = {}

    @classmethod
    def load_from_iac(cls, environment: iac_models.Environment, base_path: pathlib.Path) -> "ObjectsCollection":
        cwd = os.getcwd()
        try:
            os.chdir(base_path.as_posix())

            result = cls()

            for cluster_file_name in environment.cluster_files:
                cluster = data_models.Cluster.load_from_iac(pathlib.Path(cluster_file_name))
                logging.info(f"Loaded cluster {cluster.name!r} from [bold]{cluster_file_name}[/bold]")
                if cluster.recipe.arcadia_path not in result.recipes:
                    logging.info(
                        f"Loaded cluster recipe {cluster.recipe.name!r} "
                        f"from [bold]{cluster.recipe.arcadia_path}[/bold]"
                    )
                result.clusters[cluster_file_name] = cluster
                result.recipes[cluster.recipe.arcadia_path] = cluster.recipe

            for prober_file_name in environment.prober_files:
                prober = data_models.Prober.load_from_iac(pathlib.Path(prober_file_name))
                logging.info(f"Loaded prober {prober.name!r} from [bold]{prober_file_name}[/bold]")

                effective_configs = []
                for config in prober.configs:
                    if config.cluster is None:
                        effective_configs.append(config)
                    elif config.cluster.arcadia_path in result.clusters:
                        effective_configs.append(config)

                result.probers[prober.arcadia_path] = prober.copy(
                    update={
                        "configs": effective_configs,
                    }
                )
        finally:
            os.chdir(cwd)

        return result

    @classmethod
    def load_from_api(cls, environment: iac_models.Environment):
        return cls.load_from_api_with_client(environment.get_mr_prober_api_client())

    @classmethod
    def load_from_api_with_client(cls, client: MrProberApiClient) -> "ObjectsCollection":
        result = cls()

        for recipe in client.recipes.list():
            if recipe.manually_created:
                logging.debug(
                    f"Skipped cluster recipe {recipe.name!r} with id={recipe.id}, "
                    f"because it's marked as [green italic]manually created[/green italic]"
                )
                continue

            if recipe.arcadia_path in result.recipes:
                raise StopCliProcess(
                    "[bold]Consistency is broken:[/bold] two cluster recipes in API "
                    f"(#{recipe.id} and #{result.recipes[recipe.arcadia_path].id}) "
                    f"have the same arcadia_path: {recipe.arcadia_path}",
                    ExitCode.API_CONSISTENCY_IS_BROKEN
                )

            logging.info(
                f"Loaded cluster recipe {recipe.name!r} with id={recipe.id}, arcadia_path={recipe.arcadia_path!r}"
            )
            result.recipes[recipe.arcadia_path] = data_models.Recipe.load_from_api_model(client, recipe)

        recipe_ids = {recipe.id for recipe in result.recipes.values()}

        for cluster in client.clusters.list():
            if cluster.manually_created:
                logging.debug(
                    f"Skipped cluster {cluster.name!r} with id={cluster.id}, "
                    f"because it's marked as [green italic]manually created[/green italic]"
                )
                continue

            if cluster.recipe.id not in recipe_ids:
                logging.warning("There is a manually created recipe with an IaC-created cluster. ")
                logging.warning(
                    "Please, make them consistency: "
                    "migrate the recipe into IaC configs or remove the cluster from IaC configs."
                )
                logging.warning(f"Cluster: {cluster!r}")
                logging.warning(f"Recipe: {cluster.recipe!r}")

                raise StopCliProcess(
                    "There is a manually created recipe with an IaC-created cluster. Can't continue.",
                    ExitCode.API_CONSISTENCY_IS_BROKEN
                )

            if cluster.arcadia_path in result.clusters:
                raise StopCliProcess(
                    "[bold]Consistency is broken:[/bold] two cluster in API "
                    f"(#{cluster.id} and #{result.clusters[cluster.arcadia_path].id}) "
                    f"have the same arcadia_path: {cluster.arcadia_path}",
                    ExitCode.API_CONSISTENCY_IS_BROKEN
                )

            logging.info(f"Loaded cluster {cluster.name!r} with id={cluster.id}, arcadia_path={cluster.arcadia_path!r}")
            result.clusters[cluster.arcadia_path] = data_models.Cluster.load_from_api_model(client, cluster, result)

        for prober in client.probers.list():
            if prober.manually_created:
                logging.debug(
                    f"Skipped prober {prober.name!r} with id={prober.id}, "
                    f"because it's marked as [green italic]manually created[/green italic]"
                )
                continue

            if prober.arcadia_path in result.probers:
                raise StopCliProcess(
                    "[bold]Consistency is broken:[/bold] two probers in API "
                    f"(#{prober.id} and #{result.probers[prober.arcadia_path].id}) "
                    f"have the same arcadia_path: {prober.arcadia_path}",
                    ExitCode.API_CONSISTENCY_IS_BROKEN
                )

            logging.info(f"Loaded prober {prober.name!r} with id={prober.id}, arcadia_path={prober.arcadia_path!r}")
            result.probers[prober.arcadia_path] = data_models.Prober.load_from_api_model(client, prober, result)

        return result
