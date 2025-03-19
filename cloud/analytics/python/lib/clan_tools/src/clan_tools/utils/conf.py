import typing as tp
import yaml  # type: ignore


def read_conf(path: str) -> tp.Dict[str, tp.Any]:
    with open(path, 'r') as f:
        config = yaml.load(f, Loader=yaml.FullLoader)
    return config

__all__ = ['read_conf']
