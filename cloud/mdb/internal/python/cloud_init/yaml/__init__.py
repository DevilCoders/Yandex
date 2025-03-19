import yaml
from .cloud_config import Dumper
from ..config import CloudConfig


def dump_cloud_config(config: CloudConfig) -> str:
    result = config.headers() + '\n'
    result += yaml.dump(config, Dumper=Dumper, sort_keys=False, indent=4, canonical=False)
    return result
