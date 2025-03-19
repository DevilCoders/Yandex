"""
Config functions
"""

import os
import threading
import library.python.resource as resource

import yaml


_loaded_config: dict = {}
_config_lock = threading.Lock()


def _guess_config_path() -> str:
    if config_path := os.environ.get('DBM_CONFIG'):
        return config_path
    if arcadia_source_root := os.environ.get('ARCADIA_SOURCE_ROOT'):
        return os.path.join(arcadia_source_root, 'cloud/mdb/dbm/app.yaml')
    return 'app.yaml'


def app_config() -> dict:
    """
    Load application config
    """
    global _loaded_config
    if not _loaded_config:
        with _config_lock:
            config_path = _guess_config_path()
            if os.path.exists(config_path):
                with open(config_path) as fd:
                    _loaded_config = yaml.safe_load(fd)
            else:
                print('Using built-in config')
                _loaded_config = yaml.safe_load(resource.find('config/default/app.yaml'))

    return _loaded_config
