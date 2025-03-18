import yatest.common
import library.python.testing.pytest_runner.runner as pytest_runner


def add_prefix_cb(prefix):
    def _subtest_cb(subtest):
        subtest["name"] = prefix + "::" + subtest["name"]
        print("return subtest:%s" % subtest)
        return subtest

    return _subtest_cb


def run_pytest():
    pytest_runner.run(
        [
            yatest.common.source_path("devtools/dummy_arcadia/pytests-samples/test0.py"),
        ],
        pytest_bin=yatest.common.build_path("devtools/dummy_arcadia/pytest/pytest"),
        subtest_cb=add_prefix_cb('DynamicPrefixName')
    )
