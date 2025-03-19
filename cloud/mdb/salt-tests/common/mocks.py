from functools import reduce
from typing import Mapping
from copy import deepcopy

from pkg_resources import parse_version


def mock_pillar(salt, pillar=None):
    pillar_store = {} if pillar is None else pillar

    def _pillar_get(path, default=KeyError):
        result = _find_by_path(pillar_store, path, default)
        if result is KeyError:
            raise KeyError('Pillar key not found: {0}'.format(path))

        return result

    salt['pillar.get'] = _pillar_get


def mock_grains(salt, grains=None):
    grains_store = {} if grains is None else grains

    def _pillar_get(path, default=None):
        return _find_by_path(grains_store, path, default)

    salt['grains.get'] = _pillar_get


def mock_vtype(salt, vtype):
    from cloud.mdb.salt.salt._modules import dbaas

    mock_pillar(
        dbaas.__salt__,
        {
            'data': {
                'dbaas': {
                    'vtype': vtype,
                },
            },
        },
    )

    salt['dbaas.installation'] = dbaas.installation
    salt['dbaas.is_aws'] = dbaas.is_aws
    salt['dbaas.is_compute'] = dbaas.is_compute
    salt['dbaas.is_porto'] = dbaas.is_porto


def merge_dicts(*args):
    def merge(a, b, path=None):
        path = [] if path is None else path
        for key in b:
            if key in a:
                if isinstance(a[key], dict) and isinstance(b[key], dict):
                    merge(a[key], b[key], path + [str(key)])
                else:
                    a[key] = b[key]
            else:
                if isinstance(b[key], dict):
                    a[key] = deepcopy(b[key])
                else:
                    a[key] = b[key]
        return a

    return reduce(merge, args, {})


def mock_grains_filterby(salt):
    def _filter_by(conf, merge=None):
        merge_store = {} if merge is None else merge
        # raise Exception(conf['Debian'], merge_store)
        merged = merge_dicts(conf['Debian'], merge_store)
        return merged

    salt['grains.filter_by'] = _filter_by


def _find_by_path(store, path, default):
    result = store
    for key in path.split(':'):
        if isinstance(result, Mapping):
            if key in result:
                result = result[key]
                continue

            return default

        try:
            idx = int(key)
            if len(result) > idx:
                result = result[idx]
                continue
        except Exception:
            pass

        return default

    return result


def mock_version_cmp(salt):
    def _version_cmp(v1_str, v2_str):
        v1 = parse_version(v1_str)
        v2 = parse_version(v2_str)

        if v1 > v2:
            return 1

        if v1 < v2:
            return -1

        return 0

    salt['pkg.version_cmp'] = _version_cmp


def mock_get_md5_encrypted_password(user, password):
    return ''
