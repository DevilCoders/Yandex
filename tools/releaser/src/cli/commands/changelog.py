# coding: utf-8

import os
import codecs

import click
from gitchronicler import chronicler

from tools.releaser.src.cli import options, utils
from tools.releaser.src.lib import changelog_converter
from tools.releaser.src.lib.vcs import get_vcs_ctl


@click.command(
    help='collect git commits in changelog.md',
)
@options.non_interactive_option
@options.version_option
@options.version_schema
@options.release_type
@options.pull_vcs_option
@options.dry_run_option
def changelog(non_interactive=None, version=None, version_schema=None, release_type=None, pull_vcs=None, dry_run=None):
    """
      1. Подсмотреть предыдущую версию в changelog.md
      2. Найти коммиты с этой версии до текущей, используя тэг в git
      3. Открыть файл changelog.md на редактирование, добавив туда эти коммиты
         Если пользователь сохранит изменения — продолжить,
         Если выйдет без сохранения — закончить работу, не сохранять изменения.

      с флагом --non_interactive не открывает редактор, а просто дописывает
      новую ченжлог-запись.
    """
    if pull_vcs:
        get_vcs_ctl(dry_run=dry_run).pull()

    new_record = chronicler.get_changelog_record(to_version=version, versioning_schema=version_schema,
                                                 release_type=release_type)

    with codecs.open(chronicler.changelog_path, 'r+', 'utf-8') as changelog_file:
        old_records = changelog_file.read()
        updated_changelog = new_record + old_records

        if not non_interactive:
            updated_changelog = click.edit(text=updated_changelog)
            if updated_changelog is None:
                msg = 'File was not saved. Aborting.'
                raise click.ClickException(msg)

        if dry_run:
            click.echo('[DRY RUN] Writing updated changelog in ' + chronicler.changelog_path)
        else:
            changelog_file.seek(0)
            changelog_file.write(updated_changelog)
    return chronicler.get_current_version()


@click.command(
    help='convert debian changelog in changelog.md',
)
@options.trace_option
@options.dry_run_option
def convert_changelog(trace, dry_run):
    deb_changelog_path = './debian/changelog'
    if os.path.isfile(chronicler.changelog_path):
        msg = "The new changelog file exists. Abort."
        raise click.ClickException(msg)

    if dry_run:
        click.echo('[DRY RUN] Converting %s in %s' % (
            deb_changelog_path,
            chronicler.changelog_path
        ))
    else:
        changelog_converter.convert_changelog(
            debian_changelog=deb_changelog_path,
            markdown_changelog=chronicler.changelog_path,
            trace=trace
        )
