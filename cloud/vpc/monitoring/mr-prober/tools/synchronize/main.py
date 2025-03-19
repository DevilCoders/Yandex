#!/usr/bin/env python3
import logging
import os
import pathlib
import sys
import tempfile
from typing import Optional, List

import click
from rich.prompt import Confirm

from tools.common import StopCliProcess, ExitCode, console, cli, main, print_failure_panel, print_success_panel, \
    print_warning_panel, read_api_key
from tools.synchronize import examples
from tools.synchronize.cli import get_environment, get_object_collections, DiffPrinter, get_config
from tools.synchronize.diff import DiffApplier, DiffBuilder, ApplyCanceled
from tools.synchronize.export import ExportableObjectType, export_objects
from tools.synchronize.iac.models import Environment


def _common_options_decorator(fn):
    fn = click.option(
        "-c", "--config", "config_name", default="config.yaml",
        help="path to config.yaml which describes all Mr. Prober entities", show_default=True, type=str
    )(fn)
    fn = click.option(
        "-e", "--environment", "environment_name", required=True,
        help="environment name (i.e. prod or preprod). See config.yaml for options", type=str
    )(fn)

    return fn


def _include_option_decorator(fn):
    return click.option(
        "--include", "included_paths",
        help="specific relative arcadia paths to be synchronized", type=str, multiple=True,
    )(fn)


@cli.command(short_help="show diff between IaC configs and Mr. Prober API")
@_common_options_decorator
@_include_option_decorator
def plan(config_name: str, environment_name: str, included_paths: List[str]):
    included_paths = included_paths if included_paths else None
    environment = get_environment(config_name, environment_name)
    iac_collection, api_collection = get_object_collections(environment, pathlib.Path(config_name).parent)
    diff = DiffBuilder(include=included_paths).build(iac_collection, api_collection)

    if diff.is_empty():
        print_success_panel("Diff is empty, nice!", emoji=":tada:")
    else:
        _print_diff(diff, iac_collection)


@cli.command(short_help="apply diff from IaC configs into Mr. Prober API")
@_common_options_decorator
@_include_option_decorator
@click.option(
    "--api-key-file", "api_key_file",
    help="file with API KEY for Mr. Prober API. If not specified, use $API_KEY", type=str
)
@click.option("--auto-approve", "auto_approve", help="Don't ask before applying diff", is_flag=True, default=False)
def apply(config_name: str, environment_name: str, included_paths: List[str], api_key_file: Optional[str],
          auto_approve: bool = False):
    included_paths = included_paths if included_paths else None
    api_key = read_api_key(api_key_file)

    environment = get_environment(config_name, environment_name)
    iac_collection, api_collection = get_object_collections(environment, pathlib.Path(config_name).parent)
    diff = DiffBuilder(include=included_paths).build(iac_collection, api_collection)

    if diff.is_empty():
        print_success_panel("Diff is empty, nice!", emoji=":tada:")
        sys.exit(0)

    _print_diff(diff, iac_collection)

    try:
        if not auto_approve:
            if not Confirm.ask("Apply this plan?", console=console, default=False):
                raise ApplyCanceled()

        console.print()
        console.rule("[bold bright_green]Apply[/bold bright_green]")

        client = environment.get_mr_prober_api_client(api_key)

        DiffApplier(need_confirmation=not auto_approve).apply(client, api_collection, diff)
    except ApplyCanceled:
        raise StopCliProcess(
            "Apply canceled by user. Enter 'y' or 'Y' next time to apply the plan.", ExitCode.APPLY_CANCELED_BY_USER
        )

    print_success_panel("All objects are synchronized with Mr. Prober API!")


@cli.command(short_help="export manually created objects from Mr. Prober API to IaC")
@_common_options_decorator
@click.option(
    "--api-key-file", "api_key_file",
    help="file with API KEY for Mr. Prober API. If not specified, use $API_KEY", type=str
)
@click.option(
    "--unset-manually-created-flag", "unset_manually_created_flag", type=bool, is_flag=True,
    help="remove manually_created flag on exported objects in Mr. Prober API. API key is needed"
)
@click.argument("object_type", type=ExportableObjectType)
@click.argument("search_terms", metavar="<ID_OR_NAME_OR_ARCADIA_PATH_REGEXPS>", type=str, nargs=-1)
def export(
    config_name: str, environment_name: str, api_key_file: Optional[str],
    object_type: ExportableObjectType, search_terms: List[str],
    unset_manually_created_flag: bool,
):
    config_file_path = pathlib.Path(config_name)
    api_key = read_api_key(api_key_file)

    environment = get_environment(config_name, environment_name)
    client = environment.get_mr_prober_api_client(api_key)

    console.print()
    console.rule("[bold bright_green]Export[/bold bright_green]")
    console.print()

    objects = export_objects(
        console, object_type, client, set(search_terms), unset_manually_created_flag, base_path=config_file_path.parent
    )

    update_config_if_needed(config_file_path, environment_name, environment, object_type, objects)

    if not objects:
        print_failure_panel(f"Specified manually created {object_type} not found on {client}")
    elif not unset_manually_created_flag:
        print_warning_panel(
            f"All {object_type} saved! Check `arc status`. "
            f"Don't forget to re-run with --api-key-file and --unset-manually-created-flag before committing changes!"
        )
    else:
        print_success_panel(f"All {object_type} saved! Check `arc status` and commit changes")


@cli.command(short_help="print example of configs")
def example():
    example_objects = {
        "config.yaml": examples.config,
        "recipes/examples/recipe.yaml": examples.recipe,
        "clusters/prod/examples/cluster.yaml": examples.cluster,
        "probers/example/prober.yaml": examples.prober,
    }

    with tempfile.NamedTemporaryFile() as file:
        path = pathlib.Path(file.name)

        for filename, example_object in example_objects.items():
            example_object.save_to_file(path)
            console.rule(filename, style="bold red3")
            console.print(path.read_text())


def update_config_if_needed(
    config_file_path: pathlib.Path, environment_name: str, environment: Environment, object_type: ExportableObjectType,
    exported_objects: List
):
    config_updated = False

    cwd = os.getcwd()
    os.chdir(config_file_path.parent.as_posix())
    try:
        if object_type == ExportableObjectType.CLUSTERS:
            current_cluster_files = list(environment.cluster_files)
            for cluster in exported_objects:
                if cluster.arcadia_path not in current_cluster_files:
                    logging.info(
                        f"Add cluster {cluster.arcadia_path!r} to config, "
                        f"because current config doesn't contain path of this cluster"
                    )
                    environment.clusters.append(cluster.arcadia_path)
                    config_updated = True

        if object_type == ExportableObjectType.PROBERS:
            current_prober_files = list(environment.prober_files)
            for prober in exported_objects:
                if prober.arcadia_path not in current_prober_files:
                    logging.info(
                        f"Add prober {prober.arcadia_path!r} to config, "
                        f"because current config doesn't contain path of this prober"
                    )
                    environment.probers.append(prober.arcadia_path)
                    config_updated = True
    finally:
        os.chdir(cwd)

    if config_updated:
        config = get_config(config_file_path)
        config.environments[environment_name] = environment
        config.save_to_file(config_file_path)

        logging.info(f"Updated config saved to [bold]{config_file_path}[/bold]")


def _print_diff(diff, iac_collection):
    console.print()
    console.rule("[bold bright_green]Diff[/bold bright_green]")
    DiffPrinter(console).print(diff, iac_collection)


if __name__ == "__main__":
    main()
