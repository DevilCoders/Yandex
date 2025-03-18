# coding: utf-8
"""
Module to define all used `click.argument`s
"""


import click

optional_subcommand_arguments = click.argument(
    'cmd_args',
    nargs=-1,
    type=click.UNPROCESSED,
)
