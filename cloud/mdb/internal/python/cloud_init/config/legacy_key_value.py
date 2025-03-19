from .base import CloudInitModule

LEGACY_KEYS = {
    'deploy_version',
    'mdb_deploy_api_host',
}


class LegacyKeyValue(CloudInitModule):
    def __init__(self, key: str, value: str):
        if key not in LEGACY_KEYS:
            raise RuntimeError('Use this only for LEGACY_KEYS that we put in user-data.')
        self.key = key
        self.value = value

    @property
    def module_name(self) -> str:
        raise NotImplementedError('this is not a module and should be handled in particular way')
