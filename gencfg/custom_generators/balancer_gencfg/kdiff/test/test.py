import pytest
import yatest
import os


kdiff_simplify = yatest.common.source_path('gencfg/custom_generators/balancer_gencfg/kdiff/simplify.py')
cfg_path = yatest.common.runtime.work_path('configs')


@pytest.mark.parametrize('cfg', os.listdir(cfg_path))
def test(cfg):
    return yatest.common.canonical_py_execute(kdiff_simplify, args=['-i', os.path.join(cfg_path, cfg)])
