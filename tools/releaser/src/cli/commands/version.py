# coding: utf-8

import re

import click
from gitchronicler import chronicler

from tools.releaser.src.cli import options, utils
from tools.releaser.src.conf import cfg, VERSION_REGEXS
from tools.releaser.src.lib.vcs import get_vcs_ctl


@click.command(
    help='Update version file',
)
@options.dry_run_option
@options.version_option
@options.version_file_path
def version(dry_run, version, version_file_path):
    if not cfg.is_experiment_enabled('version'):
        click.echo('releaser version is disabled for now. '
                   'To enable it add "version" in `experiments` conf')
        return

    for file_path in version_file_path:
        version_regex = cfg.VERSION_FILES.get(file_path)
        version_regexs = [version_regex] if version_regex else VERSION_REGEXS

        with open(file_path, mode='r+') as version_file:
            file_content = version_file.read()

            match = None
            for version_regex in version_regexs:
                match = find_by_regex(text=file_content, pattern=version_regex)
                if match:
                    break

            if match is None:
                click.echo('No matches found in %s' % file_path)
                return

            new_version = version or chronicler.get_current_version()
            new_file_content = replace_piece(
                text=file_content,
                replacement=new_version,
                start=match.start(1),
                end=match.end(1),
            )

            if not dry_run:
                click.echo('Updating %s' % file_path)
                version_file.seek(0)
                version_file.truncate()
                version_file.write(new_file_content)
            else:
                click.echo('[DRY RUN] version file content after update:')
                click.echo(new_file_content)

        get_vcs_ctl(dry_run=dry_run).add(file_path)


def find_by_regex(text, pattern):
    matches = list(re.finditer(pattern, text))
    if len(matches) > 1:
        click.echo('Found multiple matches %s' % matches)
        # мир сложнее, чем считает скрипт, не будем ничего делать
        return
    if not len(matches):
        return
    only_match = matches[0]
    return only_match


def replace_piece(text, replacement, start, end):
    return text[:start] + replacement + text[end:]
