import os
import pytest
import yatest
from tap_test_adapter import adapter

env = os.environ.copy()
env['TEST_LIBS'] = yatest.common.source_path('contrib/nginx/tests/lib')
env['TEST_NGINX_BINARY'] = yatest.common.binary_path('nginx/strm-bin/nginx')
env['TEST_FFPROBE_BINARY'] = yatest.common.binary_path('contrib/libs/ffmpeg-3/bin/ffprobe/ffprobe')
env['TEST_FFMPEG_BINARY'] = yatest.common.binary_path('contrib/libs/ffmpeg-3/bin/ffmpeg')
env['TEST_DATA_PATH'] = yatest.common.work_path('test')

tests_root = yatest.common.test_source_path()

names = list(filter(lambda x: x[-2:] == '.t', os.listdir(tests_root)))


@pytest.mark.parametrize("name", names)
def test_simple(name):
    test = adapter(os.path.join(tests_root, name))
    test.run_test(env=env)
    assert len(test.failed) == 0, test.stderr
