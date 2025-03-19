from typing import List

import yaml


class AutomaticMinorUpdates:
    """
    Configuration of maintenance tasks for minor updates.
    """

    def __init__(self, config: dict) -> None:
        self._config = config

    @property
    def enabled(self) -> bool:
        """
        Whether automatic minor updates are enabled.
        """
        if not self._config:
            return False

        return self._config.get('enabled', True)

    @property
    def cluster_environments(self) -> List[str]:
        """
        List of cluster environments (salt environments) for which automatic minor updates are enabled. If unset,
        automatic minor updates are enabled for all cluster environments.
        """
        value = self._config.get('env', None)

        if isinstance(value, str):
            value = [value]

        return value

    @property
    def vtypes(self) -> List[str]:
        """
        Virtualization types for which automatic minor updates are enabled. If unset, automatic minor updates are
        enabled for all virtualization types.
        """
        value = self._config.get('vtype', None)

        if isinstance(value, str):
            value = [value]

        return value

    @property
    def cancel_planned_updates_to_previous_versions(self) -> bool:
        if not self.enabled:
            return False

        return self._config.get('cancel_planned_updates_to_previous_versions', False)


class AutomaticMajorUpdates:
    """
    Configuration of maintenance tasks for major updates.
    """

    def __init__(self, config: dict) -> None:
        self._config = config

    @property
    def enabled(self) -> bool:
        """
        Whether automatic major updates are enabled.
        """
        if not self._config:
            return False

        return self._config.get('enabled', True)

    @property
    def cluster_environments(self) -> List[str]:
        """
        List of cluster environments (salt environments) for which automatic major updates are enabled. If unset,
        automatic major updates are enabled for all cluster environments.
        """
        value = self._config.get('env', None)

        if isinstance(value, str):
            value = [value]

        return value

    @property
    def vtypes(self) -> List[str]:
        """
        Virtualization types for which automatic minor updates are enabled. If unset, automatic minor updates are
        enabled for all virtualization types.
        """
        value = self._config.get('vtype', None)

        if isinstance(value, str):
            value = [value]

        return value

    @property
    def target(self) -> str:
        """
        Target version.
        """
        return self._config['target_major_version']


class Version:
    """
    Version and its metadata.
    """

    def __init__(self, config: dict) -> None:
        self._config = config

    @property
    def version(self) -> str:
        """
        Version string (e.g. "19.14.13.4").
        """
        return self._config['version']

    @property
    def name(self) -> str:
        """
        Version name (e.g. "19.4 LTS").
        """
        return self._config['name']

    @property
    def downgradable(self) -> bool:
        """
        Return True if downgrade is supported.
        """
        return self._config.get('downgradable', True)

    def available(self, environment: str) -> bool:
        """
        Return True if the version is available in the specified environment.
        """
        available_envs = self._config.get('available')
        if available_envs is None:
            return True

        return environment in available_envs

    def default(self, environment: str) -> bool:
        """
        Return True if the version is default in the specified environment.
        """
        return environment in self._config.get('default', [])

    def deprecated(self, environment: str) -> bool:
        """
        Return True if the version is deprecated in the specified environment.
        """
        deprecated_envs = self._config.get('deprecated')
        if deprecated_envs is None:
            return False

        return environment in deprecated_envs

    def updatable_to(self, environment: str) -> List[str]:
        """
        List of versions that the given version can be updated to.
        """
        return self._config['updatable_to'][environment]

    @property
    def automatic_minor_updates(self) -> AutomaticMinorUpdates:
        """
        Configuration of maintenance tasks for minor updates.
        """
        return AutomaticMinorUpdates(self._config.get('automatic_minor_updates', {}))

    @property
    def automatic_major_updates(self) -> AutomaticMajorUpdates:
        """
        Configuration of maintenance tasks for major updates.
        """
        return AutomaticMajorUpdates(self._config.get('automatic_major_updates', {}))


class VersionsConfig:
    """
    Versions configuration.
    """

    def __init__(self, config: List[dict]) -> None:
        self._config = self._process(config)
        self._versions = [Version(version_config) for version_config in self._config]

    @property
    def versions(self) -> List[Version]:
        """
        List of configured versions.
        """
        return self._versions

    def default_version(self, environment: str) -> Version:
        """
        Return default version for the specified environment.
        """
        for version in self._versions:
            if version.default(environment):
                return version

        raise RuntimeError(f'Default version was not found for environment "{environment}".')

    def get_version(self, major_version: str) -> Version:
        """
        Get version metadata by major version string.
        """
        for version in self._versions:
            if version.version.startswith(f'{major_version}.'):
                return version

        raise RuntimeError(f'Major version "{major_version}" was not found.')

    @classmethod
    def _process(cls, config: List[dict]) -> List[dict]:
        cls._check_defaults(config)
        cls._fill_valid_updates(config)
        return config

    @staticmethod
    def _check_defaults(config: List[dict]) -> None:
        envs = {'porto', 'compute', 'datacloud'}
        for version_config in config:
            for env in version_config.get('default', []):
                envs.discard(env)

        if envs:
            raise RuntimeError(f'Invalid config: default version is not set for environments: "{", ".join(envs)}"')

    @staticmethod
    def _fill_valid_updates(config: List[dict]) -> None:
        def _calculate_valid_updates(src_version, env):
            valid_updates = []
            downgrade = False
            for dst_version in reversed(config):
                available = env in dst_version.get('available', [env])
                deprecated = env in dst_version.get('deprecated', [])

                if src_version == dst_version:
                    downgrade = True
                else:
                    if available and not deprecated:
                        valid_updates.insert(0, dst_version['version'])

                if downgrade and not dst_version.get('downgradable', True):
                    break

            return valid_updates

        for version in config:
            version['updatable_to'] = {}
            for env in ('porto', 'compute', 'datacloud'):
                version['updatable_to'][env] = _calculate_valid_updates(version, env)


def get_versions_config(config_file='versions.yaml') -> VersionsConfig:
    """
    Parse versions.yaml and return versions configuration.
    """
    with open(config_file, 'r') as file:
        return VersionsConfig(yaml.safe_load(file))
