# coding: utf-8
"""
Script that rename you migration.

Expect that your current branch on master
"""

import logging
import os.path
import re
import subprocess
import sys
from argparse import ArgumentParser
from collections import namedtuple

from colorlog import ColoredFormatter

log = logging.getLogger(__name__)  # pylint: disable=invalid-name

MIGRATIONS_DIR = 'migrations/'

MIGRATIONS_DIR_PATH = os.path.join(
    os.path.dirname(__file__),
    os.path.pardir,
    MIGRATIONS_DIR,
)

MASTER = 'master'


def git_command(args, check_retcode=True):
    """
    Run git
    """
    cmd_args = ['git'] + list(args)
    log.debug('Execute %r', cmd_args)
    cmd = subprocess.Popen(cmd_args, stdout=subprocess.PIPE)
    out, _ = cmd.communicate()

    if check_retcode:
        assert cmd.returncode == 0, 'Command %r exit with %r code' % (cmd_args, cmd.returncode)
    return str(out, encoding='utf-8'), cmd.returncode


def simple_git_command(command, *args):
    """
    git command wrapper
    """
    return git_command([command] + list(args))[0]


def get_current_branch():
    """
    get current branch name
    """
    out = simple_git_command('branch')
    for line in out.split('\n'):
        if line.startswith('*'):
            return line.lstrip('*').strip()
    raise RuntimeError("Can't find current branch name in %s" % out)


def is_branch_on_master(branch):
    """
    Return true if current branch on current master
    """
    _, retcode = git_command(['merge-base', '--is-ancestor', 'master', branch], check_retcode=False)
    return retcode != 0


MigrationInfo = namedtuple('MigrationInfo', ['version', 'filename'])


def get_migrations(branch):
    """
    Get migrations from migrations dir
    """
    migrations = []
    ls_tree_out = simple_git_command('ls-tree', branch, MIGRATIONS_DIR_PATH)
    for line in ls_tree_out.split('\n'):
        if not line:
            continue
        migration_file = line.split()[-1]
        if not migration_file.endswith('.sql'):
            log.debug('Got strange file - %s', migration_file)
            continue
        log.debug('Got %r file from %s line', migration_file, line)
        match = re.search(r'V(?P<version>\d+)__', migration_file)
        if match is None:
            log.warning('Can\'t extract version from %s', migration_file)
            continue
        migrations.append(MigrationInfo(int(match.group('version')), migration_file))
    return migrations


def find_migrations_to_rename(master_migrations, head_migrations):
    """
    Find megration that we should rename
    """
    lastest_master_migration = max(master_migrations, key=lambda m: m.version)

    log.info('Latest master migration is %r', lastest_master_migration)
    uncommon_migrations = sorted(set(head_migrations) - set(master_migrations))

    for migration_offset, head_migration in enumerate(uncommon_migrations, 1):
        log.debug('migration offest: %r, head miraration : %r', migration_offset, head_migration)
        new_version = lastest_master_migration.version + migration_offset
        if new_version != head_migration.version:
            yield (head_migration, new_version)


def find_when_added(migration_file):
    """
    Find when file added
    """
    out = simple_git_command('log', '--format=%H', '--diff-filter=A', '--', migration_file)
    return out.strip()


def git_checkout_new_branch(commit, branch_name):
    """
    Check commit as new bran
    """
    simple_git_command('checkout', commit, '-B', branch_name)


def git_checkout(branch):
    """
    Checkout existed branch
    """
    simple_git_command('checkout', branch)


def git_move_file(from_file, to_file):
    """
    git mv file
    """
    simple_git_command('mv', from_file, to_file)


def git_commit_amend():
    """
    update last commit
    """
    simple_git_command('commit', '--amend', '--no-edit')


def git_rebase_onto(commit, branch):
    """
    git rebase onto
    """
    simple_git_command('rebase', commit, '--onto=%s' % branch)


def git_delete_branch(branch):
    """
    delete branch (without force)
    """
    simple_git_command('branch', '-d', branch)


def build_new_name_for_migration(migration_file, new_version):
    """
    Create new filename for migration
    """
    new_filename = re.sub(r'V\d+__', 'V{0:04}__'.format(new_version), migration_file)
    assert new_filename != migration_file
    return new_filename


def rename_migration(migration, new_version):
    """
    Rename migration to new version
    """
    log.info('Should set new version %d for %r', new_version, migration)
    start_at_branch = get_current_branch()
    migration_added_at = find_when_added(migration.filename)
    rename_branch = 'rename-migration-%d-to-%d' % (migration.version, new_version)
    git_checkout_new_branch(migration_added_at, rename_branch)
    new_migration_file = build_new_name_for_migration(migration.filename, new_version)
    git_move_file(migration.filename, new_migration_file)
    git_commit_amend()
    git_checkout(start_at_branch)
    git_rebase_onto(migration_added_at, rename_branch)
    git_delete_branch(rename_branch)


def _init_logging(verbose):
    formatter = ColoredFormatter(
        '%(asctime)s %(log_color)s%(levelname)-8s:' '%(lineno)d %(module)s.%(funcName)s: %(message)s',
        log_colors={
            'SQL': 'cyan',
            'OSQL': 'blue',
            'INFO': 'green',
            'WARNING': 'yellow',
            'ERROR': 'red',
            'CRITICAL': 'red',
        },
    )
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(formatter)
    root_logger = logging.getLogger()
    root_logger.addHandler(handler)
    root_logger.setLevel(logging.DEBUG if verbose else logging.INFO)


def main():
    """
    Main
    """
    parser = ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true', help='print debug')
    parser.add_argument('-d', '--dry-run', action='store_true', help='don\'t real rename migrations, useful with -vvv')
    args = parser.parse_args()

    _init_logging(args.verbose)

    current_branch = get_current_branch()
    if current_branch == MASTER:
        parser.error('Switch to your branch, current is master')
    if is_branch_on_master(current_branch):
        parser.error('Rebase on master first')

    for migration, new_version in find_migrations_to_rename(get_migrations(MASTER), get_migrations(current_branch)):
        log.info('Try rename %r to version: %r', migration, new_version)
        if not args.dry_run:
            rename_migration(migration, new_version)


if __name__ == '__main__':
    main()
