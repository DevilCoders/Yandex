"""
PostgreSQL pillar module
"""
from copy import deepcopy
from typing import Any, Dict, List, Optional  # noqa

from ...utils.types import Pillar
from ...utils.version import Version, format_versioned_key


class PostgresqlHostPillar:
    """
    Postgresql host pillar
    """

    PGSYNC_PATH = ['data', 'pgsync']
    PRIORITY_PATH = ['data', 'pgsync', 'priority']
    REPLICATION_SOURCE_PATH = ['data', 'pgsync', 'replication_source']
    REPLICATION_SLOTS_PATH = ['data', 'use_replication_slots']
    PG_CONF_PATH = ['data', 'config']

    def __init__(self, pillar: Pillar) -> None:
        self._pillar = pillar if pillar is not None else dict()

    def _set_value(self, path: List[str], value: Any) -> None:
        subdict = self._pillar
        for key in path[:-1]:
            if key not in subdict:
                subdict[key] = dict()
            assert isinstance(subdict[key], dict)
            subdict = subdict[key]
        subdict[path[-1]] = value

    def _get_value(self, path: List[str]) -> Any:
        """
        Returns empty dict if path do not exists
        """
        subdict = self._pillar
        for key in path:
            if key not in subdict:
                return None
            subdict = subdict[key]
        return subdict

    def as_dict(self) -> Dict:
        """
        Returns pillar as dict
        """
        return deepcopy(self._pillar)

    def set_repl_source(self, repl_source: str) -> None:
        """
        Set `replication_source` option
        """
        # With specified `replication_source` we use replication
        # without replication slots
        if repl_source:
            use_slots = False  # type: Optional[bool]
        else:
            use_slots = None
            # Need to erase all host settings
            self._set_value(self.PG_CONF_PATH, None)
        self._set_value(self.REPLICATION_SOURCE_PATH, repl_source)
        self._set_value(self.REPLICATION_SLOTS_PATH, use_slots)

    def get_repl_source(self) -> str:
        """
        Get `replication_source` for host
        """
        return self._get_value(self.REPLICATION_SOURCE_PATH)

    def set_priority(self, priority: int) -> None:
        """
        Set pgsync priority for host
        """
        self._set_value(self.PRIORITY_PATH, priority)

    def get_config(self) -> Dict:
        """
        Returns postgresql host config
        """
        return self._get_value(self.PG_CONF_PATH)

    def set_config(self, config: Dict) -> None:
        """
        Set pg config host
        """
        self._set_value(self.PG_CONF_PATH, config)

    def get_priority(self) -> int:
        """
        Returns pgsync priority
        """
        return self._get_value(self.PRIORITY_PATH)

    def format_options(self, version: Version) -> Dict:
        """
        Returns options in spec format
        """
        options = dict()  # type: Dict
        pgsync_config = self._get_value(self.PGSYNC_PATH)
        if pgsync_config:
            options.update(pgsync_config)
        postgres_config = self.get_config()
        if postgres_config:
            config_key = format_versioned_key('postgresql_config', version)
            options['config_spec'] = {config_key: postgres_config}
        return options
