"""
persist_clusters:
    - metadb
persist_data_for_clusters:
    - metadb
persistent_state_dir: /path/to/state
state:
    metadb:
        name: metadb
        pg_dir: /path/to/state/pg
        work_dir: /path/to/state/my_wd
        socket_dir: /path/to/state/my_socket
        port: 5432
        pid: 1488
        config: {...}
"""
import os
import pathlib
import textwrap
from dataclasses import dataclass, asdict, field
from pathlib import Path
from typing import Any, ClassVar, Dict, List, Optional
import yaml


@dataclass
class ClusterState:
    name: str
    pg_dir: str
    work_dir: str
    socket_dir: str
    port: int
    pid: int
    config: Dict[str, Any]


@dataclass
class PersistenceConfig:
    persist_clusters: List[str] = field(default_factory=list)
    persist_data_for_clusters: List[str] = field(default_factory=list)
    persistent_state_dir: str = '/tmp'
    state: Dict[str, ClusterState] = field(default_factory=dict)

    DEFAULT_CONFIG_PATH: ClassVar[Path] = pathlib.Path('~/.mdb_pg_recipe_persistence/config.yaml').expanduser()
    DEFAULT_CONFIG_PATH_ENV: ClassVar[str] = 'MDB_POSTGRESQL_RECIPE_PERSISTENCE_CONFIG_PATH'

    @staticmethod
    def default_config_path() -> Path:
        return pathlib.Path(
            os.getenv(PersistenceConfig.DEFAULT_CONFIG_PATH_ENV) or PersistenceConfig.DEFAULT_CONFIG_PATH
        ).expanduser()

    @classmethod
    def read(cls, path: Path = None) -> Optional['PersistenceConfig']:
        path = path or cls.default_config_path()
        if not path.exists():
            return
        with path.open() as fd:
            data = yaml.safe_load(fd)
        return cls(state={name: ClusterState(**state) for name, state in data.pop('state', {}).items()}, **data)

    def write(self, path: Path = None):
        path = path or self.default_config_path()
        with path.open('w') as fd:
            yaml.safe_dump(asdict(self), fd)

    def make_shell_helper(self, path: Path = None):
        helper_path = path or self.default_config_path().parent / 'env.sh'
        with open(helper_path, 'w') as fd:
            helper_lines = (
                ['#! /bin/sh -']
                + [
                    textwrap.dedent(
                        f'''
                        export {cluster.name.upper()}_POSTGRESQL_RECIPE_HOST="{cluster.socket_dir}"
                        export {cluster.name.upper()}_POSTGRESQL_RECIPE_PORT="{cluster.port}"
                    '''
                    )
                    for cluster in self.state.values()
                ]
                + ['$@']
            )
            fd.write('\n'.join(helper_lines))
        os.chmod(helper_path, 0o755)
