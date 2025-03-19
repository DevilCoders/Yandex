import logging
import pathlib
from typing import List, Union, Literal, Tuple, Dict

import inflect
import json
from rich.console import Console
from rich.table import Table

import api.models
import database.models
import tools.synchronize.data.models as data_models
import tools.synchronize.iac.models as iac_models
from tools.common import StopCliProcess, ExitCode
from tools.synchronize.models import VariableType
from tools.synchronize.equalable import EqualableMixin
from tools.synchronize.data.collection import ObjectsCollection
from tools.synchronize.diff import Diff
from tools.synchronize.iac.errors import IacDataError

__all__ = ["DiffPrinter", "get_object_collections", "get_environment", "get_config"]


class DiffPrinter:
    def __init__(self, console: Console):
        self._console = console
        self._inflect = inflect.engine()

    def print(self, diff: Diff, iac_collection: ObjectsCollection):
        self._print_recipes(diff, iac_collection)
        self._print_clusters(diff, iac_collection)
        self._print_probers(diff, iac_collection)

    def _print_recipes(self, diff: Diff, iac_collection: ObjectsCollection):
        table = Table(
            "Id", "Name (path)", "Description", "Files", "Clusters",
            title="Recipes", title_style="bright_white bold", title_justify="left",
            leading=1,
        )
        for recipe in diff.recipes.added:
            recipe_clusters = self._get_recipe_clusters(iac_collection, recipe)
            table.add_row(
                "⋅",
                f"[bold green3]{recipe.name}[/bold green3] ({recipe.arcadia_path})",
                recipe.description,
                self._render_file_list(recipe.files, "create"),
                self._render_cluster_list(recipe_clusters),
            )

        for recipe in diff.recipes.deleted:
            recipe_clusters = self._get_recipe_clusters(iac_collection, recipe)
            table.add_row(
                str(recipe.id),
                f"[bold red3]- {recipe.name}[/bold red3] ({recipe.arcadia_path})",
                recipe.description,
                self._render_file_list(recipe.files, "delete"),
                self._render_cluster_list(recipe_clusters),
            )

        for (current_recipe, new_recipe) in diff.recipes.changed:
            new_name = ""
            if current_recipe.name != new_recipe.name:
                new_name = f"→ [bold yellow3]{new_recipe.name}[/bold yellow3]"
            recipe_clusters = self._get_recipe_clusters(iac_collection, new_recipe)
            table.add_row(
                str(current_recipe.id),
                f"[bold yellow3]~ {current_recipe.name}[/bold yellow3] {new_name} ({current_recipe.arcadia_path})",
                self._render_string_diff(current_recipe.description, new_recipe.description),
                self._render_files_diff(current_recipe.files, new_recipe.files),
                self._render_cluster_list(recipe_clusters),
            )

        if table.row_count:
            self._console.print(table, new_line_start=True)

    def _print_clusters(self, diff: Diff, iac_collection: ObjectsCollection):
        table = Table(
            "Id", "Name (path)", "Slug", "Recipe", "Variables", "Deploy Policy",
            title="Сlusters", title_style="bright_white bold", title_justify="left",
            leading=1,
        )
        for cluster in diff.clusters.added:
            table.add_row(
                "⋅",
                f"[bold green3]+ {cluster.name}[/bold green3] ({cluster.arcadia_path})",
                f"{cluster.slug}",
                f"[magenta3]{cluster.recipe.name}[/magenta3] ({cluster.recipe.arcadia_path})",
                self._render_variable_list(cluster.variables, "create"),
                self._render_cluster_deploy_policy(cluster.deploy_policy),
            )

        for cluster in diff.clusters.deleted:
            table.add_row(
                str(cluster.id),
                f"[bold red3]- {cluster.name}[/bold red3] ({cluster.arcadia_path})",
                f"{cluster.slug}",
                f"[magenta3]{cluster.recipe.name}[/magenta3] ({cluster.recipe.arcadia_path})",
                self._render_variable_list(cluster.variables, "delete"),
                self._render_cluster_deploy_policy(cluster.deploy_policy)
            )

        for (current_cluster, new_cluster) in diff.clusters.changed:
            new_name, new_recipe = "", ""
            if current_cluster.name != new_cluster.name:
                new_name = f"→ [bold yellow3]{new_cluster.name}[/bold yellow3]"
            recipe_changed = current_cluster.recipe.arcadia_path != new_cluster.recipe.arcadia_path
            if recipe_changed:
                new_recipe = f"→ [magenta3]{new_cluster.recipe.name}[/magenta3] ({new_cluster.recipe.arcadia_path})"

            deploy_policy_changed = current_cluster.deploy_policy != new_cluster.deploy_policy

            table.add_row(
                str(current_cluster.id),
                f"[bold yellow3]~ {current_cluster.name}[/bold yellow3] {new_name} ({current_cluster.arcadia_path})",
                self._render_string_diff(current_cluster.slug, new_cluster.slug),
                f"[magenta3]{current_cluster.recipe.name}[/magenta3] ({current_cluster.recipe.arcadia_path}) {new_recipe}",
                self._render_variables_diff(current_cluster.variables, new_cluster.variables),
                "Current — " + self._render_cluster_deploy_policy(current_cluster.deploy_policy) +
                "\n\nNew — " + self._render_cluster_deploy_policy(new_cluster.deploy_policy) if deploy_policy_changed
                else self._render_cluster_deploy_policy(current_cluster.deploy_policy),
            )

        if table.row_count:
            self._console.print(table, new_line_start=True)

    def _print_probers(self, diff: Diff, iac_collection: ObjectsCollection):
        table = Table(
            "Id", "Name (path)", "Files", "Runner", "Configs",
            title="Probers", title_style="bright_white bold", title_justify="left",
            leading=1,
        )
        for prober in diff.probers.added:
            table.add_row(
                "⋅",
                f"[bold green3]+ {prober.name}[/bold green3] ({prober.arcadia_path})",
                self._render_file_list(prober.files, "create"),
                str(prober.runner),
                self._render_prober_config_list(prober.configs),
            )

        for prober in diff.probers.deleted:
            table.add_row(
                str(prober.id),
                f"[bold red3]- {prober.name}[/bold red3] ({prober.arcadia_path})",
                self._render_file_list(prober.files, "delete"),
                str(prober.runner),
                self._render_prober_config_list(prober.configs),
            )

        for (current_prober, new_prober) in diff.probers.changed:
            new_name, new_runner = "", ""
            if current_prober.name != new_prober.name:
                new_name = f"→ [bold yellow3]{new_prober.name}[/bold yellow3]"
            if current_prober.runner != new_prober.runner:
                new_runner = f"\n[bold yellow3]↓[/bold yellow3]\n{new_prober.runner}"

            configs_changed = not EqualableMixin.equal_sets(current_prober.configs, new_prober.configs)

            table.add_row(
                str(current_prober.id),
                f"[bold yellow3]~ {current_prober.name}[/bold yellow3] {new_name} ({current_prober.arcadia_path})",
                self._render_files_diff(current_prober.files, new_prober.files),
                f"{current_prober.runner} {new_runner}",
                "Current — " + self._render_prober_config_list(current_prober.configs) +
                "\n\nNew — " + self._render_prober_config_list(new_prober.configs) if configs_changed
                else self._render_prober_config_list(current_prober.configs),
            )

        if table.row_count:
            self._console.print(table, new_line_start=True)

    def _render_file_list(
        self, files: List[Union[data_models.RecipeFile, data_models.ProberFile]], mode: Literal["create", "delete"]
    ) -> str:
        result: List[str] = []
        for file in files:
            if mode == "create":
                file_size = len(file.content)
                result.append(
                    f"[green3]+ {file.relative_file_path}[/green3]: "
                    f"{file_size} {self._inflect.plural('byte', file_size)}"
                )
            else:
                result.append(f"[red3]- {file.relative_file_path}[/red3]")
        return "\n".join(result)

    @staticmethod
    def _render_variable_list(variables: Dict[str, VariableType], mode: Literal["create", "delete"]) -> str:
        result: List[str] = []
        for variable_name, variable_value in variables.items():
            if mode == "create":
                result.append(f"[green3]+ {variable_name}[/green3]: {variable_value!r}")
            else:
                result.append(f"[red3]- {variable_name}[/red3]")
        return "\n".join(result)

    @staticmethod
    def _render_cluster_list(clusters: List[data_models.Cluster]) -> str:
        result: List[str] = []
        if not clusters:
            result.append("[italic](empty)[/italic]")
        for cluster in clusters:
            result.append(f"[magenta3]{cluster.name}[/magenta3] ({cluster.arcadia_path})")
        return "\n".join(result)

    def _render_prober_config_list(self, configs: List[data_models.ProberConfig]) -> str:
        configs_header = f"[bold]{len(configs)} {self._inflect.plural('config', len(configs))}[/bold]"
        if configs:
            configs_header += ":"

        result: List[str] = [configs_header]

        for config in configs:
            result.extend(map(lambda line: "  " + line, self._render_prober_config(config)))

        return "\n".join(result)

    def _render_cluster_deploy_policy(self, deploy_policy: api.models.ClusterDeployPolicy) -> str:
        result = ""
        if deploy_policy is None:
            result += "[italic](empty)[/italic]"
        else:
            if deploy_policy.type == database.models.ClusterDeployPolicyType.MANUAL:
                ship = "will be shipped ASAP" if deploy_policy.ship else "will not be shipped now"
                result += f"run terraform plan/apply MANUALLY. Cluster {ship}.\n" \
                          f"Number of concurrent operations: [bright_cyan]{deploy_policy.parallelism}[/bright_cyan], " \
                          f"plan timeout: [bright_cyan]{deploy_policy.plan_timeout}[/bright_cyan] {self._inflect.plural('second', deploy_policy.plan_timeout)}, " \
                          f"apply timeout: [bright_cyan]{deploy_policy.apply_timeout}[/bright_cyan] {self._inflect.plural('second', deploy_policy.apply_timeout)}\n"
            else:
                result += f"run terraform plan/apply every [bright_cyan]{deploy_policy.sleep_interval}[/bright_cyan] {self._inflect.plural('second', deploy_policy.sleep_interval)} " \
                          f"Number of concurrent operations: [bright_cyan]{deploy_policy.parallelism}[/bright_cyan], " \
                          f"plan timeout: [bright_cyan]{deploy_policy.plan_timeout}[/bright_cyan] {self._inflect.plural('second', deploy_policy.plan_timeout)}, " \
                          f"apply timeout: [bright_cyan]{deploy_policy.apply_timeout}[/bright_cyan] {self._inflect.plural('second', deploy_policy.apply_timeout)}\n"
        return result.rstrip()

    def _render_prober_config(self, config: data_models.ProberConfig) -> List[str]:
        config_status = f"[red3]Disabled[/red3]"
        if config.is_prober_enabled:
            config_status = f"[green3]Enabled[/green3]"

        if config.cluster is not None:
            cluster = f"[magenta3]{config.cluster.name}[/magenta3] ({config.cluster.arcadia_path})"
        else:
            cluster = f"[magenta3]any cluster[/magenta3]"

        result = [f"— {config_status} on {cluster}"]
        if config.is_prober_enabled or config.matrix_variables or config.variables:
            result[-1] += ":"

        if config.is_prober_enabled:
            if config.hosts_re is not None:
                result.append(f"  hosts regexp: [magenta3]/{config.hosts_re}/i[/magenta3]")
            result.append(
                f"  run every [bright_cyan]{config.interval_seconds}[/bright_cyan] "
                f"{self._inflect.plural('second', config.interval_seconds)}"
            )
            result.append(
                f"  with timeout [bright_cyan]{config.timeout_seconds}[/bright_cyan] "
                f"{self._inflect.plural('second', config.timeout_seconds)}"
            )
            result.append(f"  upload logs policy [bright_cyan]{config.s3_logs_policy}[/bright_cyan]")
            if config.default_routing_interface:
                result.append(
                    f"  default routing interface [bright_cyan]{config.default_routing_interface}[/bright_cyan]"
                )
            if config.dns_resolving_interface:
                result.append(f"  DNS resolving interface [bright_cyan]{config.dns_resolving_interface}[/bright_cyan]")

        if config.matrix_variables:
            result.append(f"  matrix:")
            result.extend(map(lambda item: f"    {item[0]}: {json.dumps(item[1])}", config.matrix_variables.items()))
        if config.variables:
            result.append(f"  variables:")
            result.extend(map(lambda item: f"    {item[0]}: {json.dumps(item[1])}", config.variables.items()))
        return result

    @staticmethod
    def _render_variables_diff(
        current_variables: Dict[str, VariableType], new_variables: Dict[str, VariableType]
    ) -> str:
        result: List[str] = []
        for common_variable in current_variables.keys() & new_variables.keys():
            value_changed = current_variables[common_variable] != new_variables[common_variable]
            if value_changed:
                result.append(
                    f"[yellow3]~ {common_variable}[/yellow3]: "
                    f"{current_variables[common_variable]} [bold yellow3]→[/bold yellow3] "
                    f"{new_variables[common_variable]}"
                )
            else:
                result.append(f"[white]⋅ {common_variable}[/white]: not changed")

        for new_variable in new_variables.keys() - current_variables.keys():
            result.append(f"[green3]+ {new_variable}[/green3]: {new_variables[new_variable]}")

        for deleted_variable in current_variables.keys() - new_variables.keys():
            result.append(f"[red3]- {deleted_variable}[/red3]")

        return "\n".join(result)

    @staticmethod
    def _render_string_diff(current_string: str, new_string: str) -> str:
        if current_string == new_string:
            return f"[white]{current_string}[/white]"

        return f"{current_string} [bold yellow3]→[/bold yellow3] {new_string}"

    def _render_files_diff(
        self,
        current_files: List[Union[data_models.RecipeFile, data_models.ProberFile]],
        new_files: List[Union[data_models.RecipeFile, data_models.ProberFile]]
    ) -> str:
        result: List[str] = []

        current_files_by_path = {file.relative_file_path: file for file in current_files}
        new_files_by_path = {file.relative_file_path: file for file in new_files}

        for common_file_path in current_files_by_path.keys() & new_files_by_path.keys():
            content_changed = current_files_by_path[common_file_path].content != new_files_by_path[
                common_file_path].content
            is_executable_changed = False
            if isinstance(current_files_by_path[common_file_path], data_models.ProberFile):
                is_executable_changed = current_files_by_path[common_file_path].is_executable != new_files_by_path[
                    common_file_path].is_executable
            if content_changed:
                file_size = len(new_files_by_path[common_file_path].content)
                result.append(
                    f"[yellow3]~ {common_file_path}[/yellow3]: updated, "
                    f"{len(current_files_by_path[common_file_path].content)} [bold yellow3]→[/bold yellow3] "
                    f"{file_size} {self._inflect.plural('byte', file_size)}"
                )
            elif is_executable_changed:
                result.append(
                    f"[yellow3]~ {common_file_path}[/yellow3]: updated, executable changed "
                    f"{current_files_by_path[common_file_path].is_executable} [bold yellow3]→[/bold yellow3] "
                    f"{new_files_by_path[common_file_path].is_executable}"
                )
            else:
                result.append(f"[white]⋅ {common_file_path}[/white]: not changed")

        for new_file_path in new_files_by_path.keys() - current_files_by_path.keys():
            file_size = len(new_files_by_path[new_file_path].content)
            result.append(
                f"[green3]+ {new_file_path}[/green3]: created, {file_size} {self._inflect.plural('byte', file_size)}"
            )

        for deleted_file_path in current_files_by_path.keys() - new_files_by_path.keys():
            result.append(f"[red3]- {deleted_file_path}[/red3]: deleted")

        return "\n".join(result)

    @staticmethod
    def _get_recipe_clusters(iac_collection: ObjectsCollection, recipe: data_models.Recipe):
        return [cluster for cluster in iac_collection.clusters.values() if
                cluster.recipe.arcadia_path == recipe.arcadia_path]


def get_object_collections(
    environment: iac_models.Environment, base_path: pathlib.Path
) -> Tuple[ObjectsCollection, ObjectsCollection]:
    logging.info(f"[bold green]Loading information from IaC configs...")
    iac_collection = ObjectsCollection.load_from_iac(environment, base_path)

    logging.info(f"[bold green]Updating information from API — {environment.endpoint}...")
    api_collection = ObjectsCollection.load_from_api(environment)

    return iac_collection, api_collection


def get_environment(config_file_name: str, environment_name: str) -> iac_models.Environment:
    config = get_config(pathlib.Path(config_file_name))
    logging.debug(f"Loaded {config!r} from [bold]{config_file_name}[/bold]")

    if environment_name not in config.environments:
        logging.error(
            f"Environment [bold]{environment_name}[/bold] not found in config. "
            f"Available environments: {list(config.environments.keys())}."
        )
        raise StopCliProcess(
            f"Environment [bold]{environment_name}[/bold] not found in config", ExitCode.ENVIRONMENT_NOT_FOUND
        )

    environment = config.environments[environment_name]
    logging.debug(f"Selected environment from config: {environment!r}")

    return environment


def get_config(config_file: pathlib.Path) -> iac_models.Config:
    if not config_file.is_file():
        logging.error(
            f"File [bold]{config_file}[/bold] not found. Specify correct path in [italic]--config[/italic] option."
        )
        raise StopCliProcess(f"Config file [bold]{config_file}[/bold] not found", ExitCode.CONFIG_NOT_FOUND)

    logging.info(f"Loading configuration from [bold]{config_file}[/bold]...")
    try:
        return iac_models.Config.load_from_file(config_file)
    except IacDataError as e:
        raise StopCliProcess(f"Can't read config: {e}", ExitCode.BAD_CONFIG)
