# coding: utf-8

import click

from tools.releaser.src.cli import options
from tools.releaser.src.lib.vcs import get_vcs_ctl


@click.command(
    help='commit known files to vcs'
)
@options.version_option
@options.direct_push_option
@options.dry_run_option
def vcs_commit(version, direct_push, dry_run):
    get_vcs_ctl(version=version, direct_push=direct_push, dry_run=dry_run).commit_command()


@click.command(
    help='revert vcs commit'
)
@options.version_option
@options.direct_push_option
@options.dry_run_option
def rollback_vcs_commit(version, direct_push, dry_run):
    get_vcs_ctl(direct_push=direct_push, dry_run=dry_run).rollback_vcs_commit_command()


@click.command(
    help='vcs tag last commit with last changelog version'
)
@options.version_option
@options.dry_run_option
def vcs_tag(version, dry_run):
    get_vcs_ctl(version=version, dry_run=dry_run).tag_command()


@click.command(
    help='vcs tag last commit with last changelog version'
)
@options.git_remote_option
@options.direct_push_option
@options.dry_run_option
def vcs_push(remote, direct_push, dry_run):
    get_vcs_ctl(remote=remote, direct_push=direct_push, dry_run=dry_run).push_command()


@click.command(
    help='vcs pull changes from remote'
)
@options.dry_run_option
def vcs_pull(dry_run):
    get_vcs_ctl(dry_run=dry_run).pull()
