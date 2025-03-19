from kernel.blender.factor_storage.pylib import serialization
from kernel.blender.factor_storage.test.common import utils


def test_save_load():
    dump = serialization.compress(utils.STATIC_FACTORS, utils.DYNAMIC_FACTORS)
    error, static, dynamic = serialization.decompress(dump)
    assert error is None, 'error while decompressing: {}'.format(error)
    utils.assert_lists_equal(utils.STATIC_FACTORS, static)
    utils.assert_dynamic_features_equal(utils.DYNAMIC_FACTORS, dynamic)
