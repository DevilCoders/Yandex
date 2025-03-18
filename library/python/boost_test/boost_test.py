import os
import pytest
import logging

import yatest.common


def get_boost_test_binaries():
    depends = yatest.common.runtime._get_ya_plugin_instance().dep_roots
    for dep in depends:
        if dep != yatest.common.context.project_path:
            build_path = yatest.common.build_path(dep)
            if os.path.exists(build_path) and os.path.isdir(build_path):
                boost_binary = os.path.join(dep, os.listdir(build_path)[0])
                try:
                    res = yatest.common.execute([yatest.common.build_path(boost_binary), "--help"])
                    if "Boost.Test supports following parameters:" in res.std_err:
                        yield boost_binary
                    else:
                        logging.warning("Binary '{}' does not seem to be a boost test".format(yatest.common.build_path(boost_binary)))
                except Exception:
                    logging.exception("{} failed to be verified as a boost test binary")


@pytest.mark.parametrize("binary_path", get_boost_test_binaries(), ids=os.path.dirname)
def test(binary_path):
    yatest.common.execute([yatest.common.build_path(binary_path), "-r", "detailed"])
