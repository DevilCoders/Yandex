import os
import pytest

import yaqutils.misc_helpers as umisc


@pytest.fixture
def root_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand"))
    except:
        return str(request.config.rootdir)


@pytest.fixture
def data_path(root_path):
    return os.path.join(root_path, "serp/tests/data")


@pytest.fixture
def make_silent_run_command(monkeypatch):
    original_run_command = umisc.run_command

    def silent_run_command(cmd, stdin_fd=None, stdout_fd=None, stderr_fd=None, log_command_line=False):
        with open(os.devnull, "w") as devnull:
            original_run_command(cmd, devnull, devnull, devnull, log_command_line)

    monkeypatch.setattr(umisc, "run_command", silent_run_command)
