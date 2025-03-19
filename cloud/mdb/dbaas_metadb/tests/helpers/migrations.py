"""
Migration helpers
"""

import logging
import shutil
from pathlib import Path
from typing import Iterable, Optional
from . import queries
import yaml

log = logging.getLogger(__name__)


def get_migrations_config(project_root: Path) -> dict:
    """
    Return pgmigrate config
    """
    with (project_root / 'migrations.yml').open() as mig_fd:
        return yaml.safe_load(mig_fd)


def make_one_file_tree(copy_root: Path, project_root: Path, one_file: str, hook_dirs: tuple[str]) -> None:
    """
    Create tree for metadb.sql
    """
    if not copy_root.exists():
        copy_root.mkdir()

    config = get_migrations_config(project_root)
    if 'callbacks' not in config:
        config['callbacks'] = {}
    if 'afterAll' in config['callbacks']:
        raise RuntimeError("That helper doesn't support migration with afterAll hooks. Fix that code")
    config['callbacks']['afterAll'] = list(hook_dirs)

    # make symlinks for hooks
    for hook in hook_dirs:
        hook_path = Path(hook)
        # e.g callback like code/defs
        hook_path_parent_dirs = hook_path.parts[:-1]
        if hook_path_parent_dirs:
            (copy_root / '/'.join(hook_path_parent_dirs)).mkdir(parents=True, exist_ok=True)
        shutil.copytree(project_root / hook, copy_root / hook, dirs_exist_ok=True)

    # pgmigrate config
    with open(copy_root / 'migrations.yml', 'w') as fd:
        yaml.safe_dump(config, fd)

    copy_migrations = copy_root / 'migrations'
    copy_migrations.mkdir(exist_ok=True)

    shutil.copy(project_root / one_file, copy_migrations / 'V0001__All_in_one_migration.sql')


def list_migrations(project_root: Path) -> Iterable[Path]:
    """
    Return list of migrations files
    """
    return (project_root / 'migrations').glob('V*.sql')


def _version_from_file_path(file_path: Path) -> int:
    return int(file_path.name.split('__')[0].lstrip('V'))


def find_migration_file(project_root: Path, version: int) -> Optional[Path]:
    """
    Find migration file
    """
    for file_path in list_migrations(project_root):
        file_version = _version_from_file_path(file_path)
        if file_version == version:
            return file_path
    return None


def get_latest_migration_version(project_root: Path) -> int:
    """
    Get latest migration version
    """
    return max(_version_from_file_path(p) for p in list_migrations(project_root))


def get_applied_migration_version(dsn: str) -> int:
    """
    Get lastet applied migration version
    """
    with queries.execute_query(dsn, "SELECT max(version) AS max_ver from schema_version") as res:
        return res.fetch()[0].max_ver
