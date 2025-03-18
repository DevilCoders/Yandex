import pytest

import library.python.pytest.plugins.ya as ya_plugin


@pytest.mark.trylast
def pytest_configure(config):
    config._inicache["python_functions"] = ("run_pytest",)
    config.ya_trace_reporter = ya_plugin.DryTraceReportGenerator(None)
