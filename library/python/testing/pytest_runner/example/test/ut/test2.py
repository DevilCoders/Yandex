import yatest.common

import library.python.testing.pytest_runner.runner as pytest_runner


def run_pytest():
    pytest_runner.run(
        [
            yatest.common.source_path("devtools/dummy_arcadia/pytests-samples/suite_error.py"),
        ],
        pytest_bin=yatest.common.build_path("devtools/dummy_arcadia/pytest/pytest"),
        pytest_args=['-v',  '--junit-prefix', 'PyTestRunner']
    )
