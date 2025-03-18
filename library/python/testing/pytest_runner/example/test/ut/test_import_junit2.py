import yatest.common
import library.python.testing.pytest_runner.runner as pytest_runner


def run_pytest():
    # Import existing junit xml
    pytest_runner.import_junit_files(
        [
            yatest.common.source_path("devtools/dummy_arcadia/junit-samples/test3.xml"),
        ]
    )
