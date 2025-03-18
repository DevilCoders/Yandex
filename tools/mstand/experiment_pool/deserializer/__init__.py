from experiment_pool.deserializer import _v0, _v1
from experiment_pool.pool_exception import PoolException


def deserialize_pool(data):
    """
    :type data: dict
    :rtype: experiment_pool.pool.Pool
    """
    version = data.get('version')
    if version is None:
        ref_text = "Refer to https://wiki.yandex-team.ru/mstand/pool for details"
        raise PoolException("Pool 'version' field is not specified (actual version is 1). {}".format(ref_text))

    parsers = {
        0: _v0.deserialize_pool,
        1: _v1.deserialize_pool
    }
    if version not in parsers:
        raise Exception("Unsupported pool version: '{}'".format(version))
    return parsers[version](data)
