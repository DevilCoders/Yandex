# -*- coding: utf8 -*-

import pytest


pytest_plugins = list()
pytest_plugins += [
    'antiadblock.cryprox.tests.lib.containers_context',
    'antiadblock.cryprox.tests.lib.stub_server',
    'antiadblock.cryprox.tests.lib.config_stub',
]


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # execute all other hooks to obtain the report object
    outcome = yield
    rep = outcome.get_result()

    # set a report attribute for each phase of a call, which can
    # be "setup", "call", "teardown"

    setattr(item, "rep_" + rep.when, rep)
