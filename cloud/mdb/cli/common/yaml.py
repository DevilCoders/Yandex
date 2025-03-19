import os
from collections import OrderedDict

import yaml
import yaml.representer


def dict_representer(dumper, data):
    return yaml.representer.SafeRepresenter.represent_dict(dumper, data.items())


yaml.add_representer(dict, dict_representer)
yaml.add_representer(OrderedDict, dict_representer)


def load_yaml(file_path):
    with open(os.path.expanduser(file_path), 'r') as f:
        return yaml.safe_load(f)


def dump_yaml(data, file_path=None):
    if not file_path:
        return yaml.dump(data, default_flow_style=False, allow_unicode=True)

    with open(os.path.expanduser(file_path), 'w') as f:
        yaml.dump(data, f, default_flow_style=False, allow_unicode=True)
