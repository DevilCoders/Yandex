#!/usr/bin/env python3
import dataclasses
import glob
import sys
from typing import List

import click
from rich.table import Table

from tools.common import initialize_cli, main, console, StopCliProcess
from tools.routing_bingo.config import Environment, MrProberApiClient, load_environment, get_mr_prober_api_client
from tools.routing_bingo.solomon import SolomonClient, get_solomon_client
from tools.routing_bingo.test import load_tests, TESTS_GLOB, TestRunner


@dataclasses.dataclass
class CtxObj:
    environment: Environment
    client: MrProberApiClient
    solomon: SolomonClient


@click.group()
@click.pass_context
@click.option("-v", "--verbose", count=True)
@click.option(
    "-c", "--config", "config_path", default="config.yaml", type=str,
    help="path to config.yaml which describes all Mr. Prober entities", show_default=True,
)
@click.option(
    "-e", "--environment", "environment_name", required=True, type=str,
    help="environment name (i.e. testing or hw11-dev1). See config.yaml for options",
)
def cli(ctx, verbose: int, config_path: str, environment_name: str):
    environment = load_environment(config_path, environment_name)
    client = get_mr_prober_api_client(environment)
    solomon = get_solomon_client(environment.solomon)

    ctx.obj = CtxObj(environment, client, solomon)

    initialize_cli(verbose)


@cli.command(short_help="deploy routing-bingo with parameters of a single test run")
@click.pass_context
@click.argument("test_path", type=str, nargs=1)
@click.argument("test_run", type=str, nargs=1)
def deploy(ctx, test_path: str, test_run: str):
    """
    Redeploy cluster for a single test run.
    Useful for analyzing test run failures.
    """
    tests = load_tests([test_path], ctx.obj.environment)
    if tests[0].error:
        raise StopCliProcess(f"Error loading test. f{tests[0].error}")

    runner = TestRunner(tests[0], ctx.obj.client, ctx.obj.solomon)
    for run in tests[0].spec.runs:
        if run.name == test_run:
            runner.update_cluster(run)
            break
    else:
        raise StopCliProcess(f"Test run {test_run} not found in test {test_path}")


@cli.command(short_help="collect routing-bingo tests and print all tests and their runs")
@click.pass_context
def collect(ctx):
    tests = load_tests(glob.glob(TESTS_GLOB), ctx.obj.environment)

    table = Table(
        "Name", "Cluster", "Prober Slugs", "Runs", "Error",
        title_style="bright_white bold", leading=1,
    )
    for test in tests:
        table.add_row(
            f"[bold green]{test.name}[/bold green]",
            f"{test.spec.cluster_slug} ({test.cluster.cluster_id})",
            ", ".join(test.spec.prober_slugs),
            "\n".join("- " + (f"{run.name}" if not run.skip
                              else f"[bright_black]{run.name}[/bright_black]")
                      for run in test.spec.runs),
            test.error if test.error else "",
        )

    console.print(table)


@cli.command(short_help="run routing-bingo tests")
@click.pass_context
@click.argument(
    "test_paths", type=str, nargs=-1,
)
def run(ctx, test_paths: List[str]):
    """
    Run TESTS.

    TESTS contain paths to test_*.yaml files.
    If omitted, glob pattern 'test_*.yaml' is used
    """
    if not test_paths:
        test_paths = glob.glob(TESTS_GLOB)

    tests = load_tests(test_paths, ctx.obj.environment)

    has_failures = False
    for test in tests:
        # TODO: parallelize test runs
        runner = TestRunner(test, ctx.obj.client, ctx.obj.solomon)
        test_has_failures = runner.run()

        has_failures = has_failures or test_has_failures

    return 1 if has_failures else 0


if __name__ == "__main__":
    sys.exit(main(cli))
