import pytest
from six import iteritems

DYNAMIC_FACTORS = {
    "sp.DIVERSITY.sp_factors": [0.1, 0.2, 0.3, 0.4],
    "grp.ii.0.rf": [1, 1, 1, 1e+09],
    "grp.ii.10.gta._ComressedFactors": [0, -0.1, 0, 0.9],
    "grp.ii.5.nf.QueryURLClicksFRC": [0.5, 0.5, 0.5, 0.5],
}

STATIC_FACTORS = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.0, 0.6, 0.7, 0.8, 0.9]


def assert_lists_equal(list1, list2):
    assert list1 == pytest.approx(list2), "lists differ"


def assert_dynamic_features_equal(dyn1, dyn2):
    assert len(dyn1) == len(dyn2)
    for key, value1 in iteritems(dyn1):
        value2 = dyn2.get(key, None)
        assert value2 is not None, "Key {} not found in one of dynamic factors".format(key)
        assert_lists_equal(value1, value2)
