import pytest
import os

import yatest.common
import library.python.testing.fake_ya_package as fp


@pytest.mark.parametrize('spec, archive',
                         [('devtools/ya/package/tests/create_tarball/package.json', 'yandex-package-test.tar')])
def test_make_pkg(spec, archive):
    tarball = yatest.common.output_path(archive)
    tmp_dir = tarball + '.tmp'
    os.mkdir(tmp_dir)
    p = fp.FakeYaPackage(yatest.common.source_path(spec),
                         yatest.common.source_path,
                         yatest.common.binary_path,
                         tmp_dir)
    tarball = yatest.common.output_path(archive)
    p.make_tarball(tarball)
    assert(os.path.exists(tarball))
