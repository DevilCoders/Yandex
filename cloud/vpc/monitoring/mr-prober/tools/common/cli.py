import logging
import os
import sys
from typing import Optional

import click
from rich.console import Console
from rich.logging import RichHandler
from rich.panel import Panel

import settings
from tools.common.stop import StopCliProcess

__all__ = ["console", "starting_up", "cli", "initialize_cli", "print_success_panel",
           "print_warning_panel", "print_failure_panel", "main"]


console = Console(force_terminal=settings.RICH_FORCE_TERMINAL, width=settings.RICH_WIDTH)


def _setup_logging(level):
    logging.basicConfig(level=level, format="%(message)s", datefmt="[%X]", handlers=[RichHandler(markup=True)])
    # boto writes too many logs
    logging.getLogger("botocore").setLevel(logging.WARNING)
    logging.getLogger("s3transfer").setLevel(logging.WARNING)


def starting_up():
    console.print()
    console.rule("[bold bright_green]Starting up[/bold bright_green]")
    console.print()


def initialize_cli(verbose: int):
    if verbose >= 2:
        _setup_logging(logging.DEBUG)
    elif verbose >= 1:
        _setup_logging(logging.INFO)
        logging.info("Hint: use '-vv' for more verbose logging")
    else:
        _setup_logging(logging.WARNING)

    if verbose > 0:
        starting_up()


def print_success_panel(message: str, emoji: str = ":white_check_mark:"):
    console.print(Panel(emoji + " " + message, style="bright_green", expand=False), new_line_start=True)
    console.print()


def print_warning_panel(message: str, emoji: str = ":yellow_heart:"):
    console.print(Panel(emoji + " " + message, style="bright_yellow", expand=False), new_line_start=True)
    console.print()


def print_failure_panel(message: str, emoji: str = ":no_entry:"):
    console.print(Panel(emoji + " " + message, style="bright_red", expand=False), new_line_start=True)
    console.print()


@click.group()
@click.option("-v", "--verbose", count=True)
def cli(verbose: int):
    initialize_cli(verbose)


def main(cli_group=cli):
    try:
        cli_group()
    except Exception as e:
        if isinstance(e, StopCliProcess):
            print_failure_panel(str(e))
            sys.exit(e.exit_code)
        if not isinstance(e, SystemExit) and not isinstance(e, click.Abort):
            console.print_exception()
            console.print(
                "Hint: use '-v' or '-vv' for verbose logs. Hope, it will help to see where error occurred.",
                new_line_start=True
            )
            sys.exit(1)
