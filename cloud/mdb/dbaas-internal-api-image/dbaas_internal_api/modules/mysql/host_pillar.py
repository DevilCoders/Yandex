"""
MySQL host pillar module
"""
from copy import deepcopy
from typing import Any, Dict, List

from ...utils.types import Pillar
from ...utils.version import Version, format_versioned_key


class MysqlHostPillar:
    """
    MySQL host pillar
    """

    BACKUP_PRIORITY_PATH = ['data', 'mysql', 'backup_priority']
    PRIORITY_PATH = ['data', 'mysql', 'priority']
    REPLICATION_SOURCE_PATH = ['data', 'mysql', 'replication_source']
    MY_CONF_PATH = ['data', 'mysql', 'config']
    SERVER_ID_PATH = ['data', 'mysql', 'config', 'server-id']

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

    def _del_value(self, path: List[str]) -> None:
        subdict = self._pillar
        for key in path[:-1]:
            if key not in subdict:
                subdict[key] = dict()
            assert isinstance(subdict[key], dict)
            subdict = subdict[key]
        try:
            del subdict[path[-1]]
        except KeyError:
            pass

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

    @property
    def backup_priority(self) -> int:
        """
        Get `backup_priority` for host
        """
        return self._get_value(self.BACKUP_PRIORITY_PATH) or 0

    @backup_priority.setter
    def backup_priority(self, backup_priority: int) -> None:
        """
        Set `backup_priority` option
        """
        if backup_priority is not None:
            self._set_value(self.BACKUP_PRIORITY_PATH, backup_priority)
        else:
            self._del_value(self.BACKUP_PRIORITY_PATH)

    @property
    def priority(self) -> int:
        """
        Get `priority` for host
        """
        return self._get_value(self.PRIORITY_PATH) or 0

    @priority.setter
    def priority(self, priority: int) -> None:
        """
        Set `priority` option
        """
        if priority is not None:
            self._set_value(self.PRIORITY_PATH, priority)
        else:
            self._del_value(self.PRIORITY_PATH)

    @property
    def repl_source(self) -> str:
        """
        Get `replication_source` for host
        """
        return self._get_value(self.REPLICATION_SOURCE_PATH)

    @repl_source.setter
    def repl_source(self, repl_source: str) -> None:
        """
        Set `replication_source` option
        """
        if repl_source:
            self._set_value(self.REPLICATION_SOURCE_PATH, repl_source)
        else:
            self._del_value(self.REPLICATION_SOURCE_PATH)

    @property
    def server_id(self) -> int:
        """
        Get `server_id` config option for host
        """
        return self._get_value(self.SERVER_ID_PATH)

    @server_id.setter
    def server_id(self, server_id: int) -> None:
        """
        Set `server_id` config option for host
        """
        self._set_value(self.SERVER_ID_PATH, server_id)

    def get_config(self) -> Dict:
        """
        Returns mysql host config
        """
        return self._get_value(self.MY_CONF_PATH)

    def set_config(self, config: Dict) -> None:
        """
        Set pg config host
        """
        self._set_value(self.MY_CONF_PATH, config)

    def format_options(self, version: Version) -> Dict:
        """
        Returns options in spec format
        """
        options = {}  # type: Dict
        if self.repl_source:
            options.update({'replication_source': self.repl_source})

        if self.backup_priority is not None:
            options.update({'backup_priority': self.backup_priority})

        if self.priority is not None:
            options.update({'priority': self.priority})

        mysql_config = self.get_config()
        if mysql_config:
            config_key = format_versioned_key('mysql_config', version)
            options['config_spec'] = {config_key: mysql_config}
            # (expression has type "Dict[str, Dict[Any, Any]]", target has type "str")
        return options
