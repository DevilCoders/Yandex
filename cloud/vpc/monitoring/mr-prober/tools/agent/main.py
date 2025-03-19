#!/usr/bin/env python3
import logging
import socket
import os.path
import subprocess
import sys
import time
from typing import List

import click
import rich.table
from rich import box
from rich.progress import Progress, TextColumn, BarColumn, TimeRemainingColumn, RenderableColumn
from rich.table import Table, Column
from rich.text import Text

import settings
from agent.config import S3AgentConfigLoader
from agent.config.models import ProberWithConfig
from agent.timer import Timer
from tools.agent.info import get_instance_id, print_network_interfaces
from tools.agent.statistics import RunStatistics
from tools.common import main, cli, console, StopCliProcess

config_loader = S3AgentConfigLoader()


def get_agent_config():
    try:
        return config_loader.load(update_files_on_disk=False)
    except Exception as e:
        logging.error(f"Can't get agent config from S3: {e}")
        raise


@cli.command(short_help="information about Mr. Prober virtual machine")
def info():
    instance_id = get_instance_id()

    table = Table("Key", Column("Value", style="green3"), show_header=False, box=box.ROUNDED)
    fqdn = socket.getfqdn()
    table.add_row("Instance ID", instance_id)
    table.add_row("Instance FQDN", fqdn)
    table.add_row(
        "Hostname from [cyan3]$HOSTNAME",
        Text(settings.HOSTNAME, style="red" if fqdn != settings.HOSTNAME else None)
    )

    table.add_row("", "")
    table.add_row("Logs", settings.MR_PROBER_LOGS_PATH)
    table.add_row("Prober logs", os.path.join(settings.MR_PROBER_PROBER_LOGS_PATH, "<prober-slug>"))
    if settings.CLUSTER_ID is None:
        table.add_row("Cluster ID", "[bold red3]None[/bold red3][bright_white], expected value at [cyan3]$CLUSTER_ID")
    else:
        table.add_row("", "")
        table.add_row("Cluster ID", str(settings.CLUSTER_ID))

        config = get_agent_config()
        cluster = config.cluster
        table.add_row("Cluster Name", cluster.name)
        table.add_row("Cluster Slug", cluster.slug)

        if config.probers:
            table.add_row("\n[bold]Probers:[/bold]", "")
        _add_probers_info_to_table(config.probers, table)

    console.print(table)

    print_network_interfaces()


@cli.group(short_help="probers")
def probers():
    pass


@probers.command(name="list", short_help="print available probers list")
def probers_list():
    config = get_agent_config()

    table = Table("Key", "Value", show_header=False, box=box.SIMPLE, padding=0)
    _add_probers_info_to_table(config.probers, table)
    console.print(table)


@probers.command(name="run", short_help="run a prober")
@click.option("--runs", type=int, help="Number of runs", default=1, show_default=True)
@click.option("--interval", type=float, help="Interval between runs, seconds", default=0, show_default=True)
@click.option("--exit-on-fail", type=bool, is_flag=True, help="Exit on first fail", default=False)
@click.option("--no-prober-output", type=bool, is_flag=True, help="Suppress prober's stdout and stderr", default=False)
@click.option(
    "--output-only", type=bool, is_flag=True, help="Suppress anything except prober's stdout and stderr",
    default=False
)
@click.argument("prober_slug", type=str)
def run_prober(
    prober_slug: str, runs: int, interval: float, exit_on_fail: bool, no_prober_output: bool, output_only: bool
):
    config = get_agent_config()
    for prober_with_config in config.probers:
        if prober_with_config.prober.slug == prober_slug:
            found = prober_with_config
            break
    else:
        raise StopCliProcess(f"Prober with slug {prober_slug!r} not found. See '{sys.argv[0]} probers list'")

    prober = found.prober
    config = found.config
    logging.info(f"Using prober {prober!r}")
    config_loader.update_prober_files_on_disk_if_needed(prober)

    statistics = RunStatistics()

    progress_columns = (
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
        TimeRemainingColumn(),
        RenderableColumn(statistics)
    )
    progress = Progress(*progress_columns, console=console, disable=output_only or runs <= 1)
    with progress:
        for _ in progress.track(range(runs), description=f"Running a prober {runs} times..."):
            if not output_only:
                console.print(
                    f":eyes: Starting {str(prober.runner)!r} in {prober.files_location.as_posix()} "
                    f"with timeout {config.timeout_seconds} seconds"
                )

            timer = Timer()
            try:
                process = prober.runner.create_process(prober, config)
                # Start time measurement **after** process creating to avoid summarizing
                # of process starting time and running time
                timer.restart()

                stdout, stderr = process.communicate(timeout=config.timeout_seconds)
                success = process.returncode == 0

                color = "bright_white" if success else "red3"
                prober_message = f"[{color}]exited with code {process.returncode}[/{color}]"
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate()
                success = False
                prober_message = "[red3]timed out[/red3]"
            except Exception as e:
                logging.warning(f"Prober {prober.name!r} ({str(prober.runner)!r}) failed: {e}", exc_info=e)
                stdout, stderr = b"", b""
                success = False
                prober_message = f"[red3]failed to start: {e}[/red3]"

            if not output_only:
                console.print(
                    f"Prober {prober.slug!r} {prober_message}, "
                    f"printed to stdout {len(stdout)} bytes, to stderr {len(stderr)} bytes, elapsed {timer.get_total_seconds()} seconds"
                )
            if not no_prober_output:
                if output_only:
                    sys.stdout.buffer.write(stdout)
                    sys.stderr.buffer.write(stderr)
                else:
                    console.print()
                    stdout = _try_to_decode_bytes(stdout)
                    stderr = _try_to_decode_bytes(stderr)
                    if stdout:
                        console.print("Standard output:")
                        console.print(stdout, style="white", highlight=False)
                    if stderr:
                        console.print("Standard error:")
                        console.print(stderr, style="white", highlight=False)
                    console.print()

            if success:
                statistics.successes += 1
            else:
                statistics.fails += 1

            if not success and exit_on_fail:
                logging.info("Exiting due to --exit-on-fail and non-zero exit code")
                break

            time.sleep(interval)

    sys.exit(0 if statistics.fails == 0 else 1)


def _try_to_decode_bytes(data: bytes) -> str:
    try:
        return data.decode("utf-8").strip()
    except UnicodeDecodeError:
        return repr(data)


def _add_probers_info_to_table(probers_with_configs: List[ProberWithConfig], table: rich.table.Table):
    for index, prober_with_config in enumerate(probers_with_configs, start=1):
        prober = prober_with_config.prober
        config = prober_with_config.config
        if config.is_prober_enabled:
            config_description = f"[green3]runs[/green3] [bright_white]every [cyan3]{config.interval_seconds} seconds[/cyan3] " \
                                 f"with timeout [cyan3]{config.timeout_seconds} seconds[/cyan3], " \
                                 f"policy of logs uploading: [cyan3]{config.s3_logs_policy}[/cyan3]"
            table.add_row(f"[green3]{index}. {prober.name}[/green3] ({prober.slug})", config_description)
        else:
            table.add_row(
                f"[yellow3]{prober.id}. {prober.name}[/yellow3] ({prober.slug})",
                "[yellow3]doesn't run[/yellow3]"
            )


if __name__ == "__main__":
    main()
