import yatest.common

import library.python.testing.pytest_runner.runner as pytest_runner


def run_pytest():
    pytest_runner.run(
        [
            yatest.common.source_path("devtools/dummy_arcadia/pytests-samples/suite_error.py"),
        ],
        python_path="/usr/bin/python3.4",
    )
