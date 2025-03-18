import yatest.common
import library.python.testing.pytest_runner.runner as pytest_runner


def run_pytest():
    # Import existing junit xml
    pytest_runner.import_junit_files(
        [
            yatest.common.source_path("devtools/dummy_arcadia/junit-samples/test0.xml"),
            # yatest.common.source_path("devtools/dummy_arcadia/junit-samples/test1.xml"),
            yatest.common.source_path("devtools/dummy_arcadia/junit-samples/test0_empty_classname.xml")
        ]
    )
