import logging
import os
import tempfile
from getpass import getuser

import pytest

from qemu import *
from utils import ImageManager


def pytest_addoption(parser):
    parser.addoption(
        "--full", action="store_true", default=False,
        help="full suite execution that includes time-consuming tests"
    )
    parser.addoption(
        "--keep-on-failure", action="store_true", default=False,
        help="keep processes and data if test fails"
    )
    parser.addoption(
        "--test_data_path", type=str, default="",
        help="test data path"
    )


def pytest_configure(config):
    config.addinivalue_line("markers",
                            "full: mark test only for full suite run (that is "
                            "much longer)")
    config.addinivalue_line("markers",
                            "vhost_user: tests with vhost-user-blk-pci disk "
                            "backend, default options")
    config.addinivalue_line("markers",
                            "vhost_user_slow: tests with vhost-user-blk-pci "
                            "disk backend, requests are processed with delay")
    config.addinivalue_line("markers",
                            "vhost_user_virtio: tests with "
                            "vhost-user-virtio-blk-pci disk backend ("
                            "transitional device compatible with "
                            "virtio-blk-pci migration state)")
    config.addinivalue_line("markers",
                            "virtio_blk: tests with virtio-blk-pci disk "
                            "backend (in QEMU processing)")
    config.addinivalue_line("markers",
                            "vhost_fs: tests with virtio-fs disk backend")


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    """
    Making test error available in request fixture.
    (https://pytest.readthedocs.io/en/latest/reference.html#request)
    We need this to be able to skip cleanup on test failure and keep state for
    investigation.
    Copypasted and modified for class-scoped fixtures from
    https://pytest.readthedocs.io/en/latest/example/simple.html#making-test-result-information-available-in-fixtures
    """
    # execute all other hooks to obtain the report object
    outcome = yield
    rep = outcome.get_result()

    # set a report attribute for each phase of a call, which can
    # be "setup", "call", "teardown"
    setattr(item, "excinfo_" + rep.when, call.excinfo)
    # set attr of function.instance.class for class level fixtures
    setattr(item.parent.parent, "rep_" + rep.when, rep)
    setattr(item, "rep_" + rep.when, rep)


def pytest_collection_modifyitems(config, items):
    if config.getoption("--full"):
        # --full given in cli: do not skip time-consuming tests
        return
    full_run = pytest.mark.skip(reason="need --full option to run")
    for item in items:
        if "full" in item.keywords:
            item.add_marker(full_run)


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_setup(item):
    """
    HACK: Temporarily monkeypatch the log file formatter class and write the
    test name to file
    Copypasted with minimal modifications from
    https://github.com/pytest-dev/pytest/issues/8859
    """
    file_handler = item.config.pluginmanager.get_plugin(
        'logging-plugin').log_file_handler
    orig_formatter = file_handler.formatter

    try:
        file_handler.setFormatter(logging.Formatter())
        for line in ('', 'Running test {}'.format(item.nodeid)):
            file_handler.emit(
                logging.LogRecord('N/A', logging.INFO, 'N/A', 0, line, None,
                                  None))

    finally:
        file_handler.setFormatter(orig_formatter)

    yield


# =========== common fixtures  =========== #

@pytest.fixture(scope='class')
def boot_image(tmp_dir):
    overlay_image = os.path.join(tmp_dir, "boot.qcow2")
    # TODO: decide what to do if overlay already exists
    if os.path.exists(overlay_image):
        os.remove(overlay_image)
    ImageManager().mkimg(overlay_image, ImageManager.FEDORA33_IMAGE_ID)
    return overlay_image


@pytest.fixture(scope="class")
def boot_disk(boot_image):
    yield VirtioDisk(FileBackend(disk_id="boot",
                                 image=boot_image,
                                 image_type="qcow2"),
                     bootable=True,
                     with_iothread=True)


@pytest.fixture(scope='class')
def win_boot_image(tmp_dir):
    overlay_image = os.path.join(tmp_dir, "boot.qcow2")
    # TODO: decide what to do if overlay already exists
    if os.path.exists(overlay_image):
        os.remove(overlay_image)
    ImageManager().mkimg(overlay_image, ImageManager.WINDOWS10_IMAGE_ID)
    return overlay_image


@pytest.fixture(scope="class")
def win_boot_disk(win_boot_image):
    yield VirtioDisk(FileBackend(disk_id="boot",
                                 image=win_boot_image,
                                 image_type="qcow2"),
                     bootable=True,
                     with_iothread=True)


@pytest.fixture(scope='session')
def scratch_image(tmp_dir):
    file_path = os.path.join(tmp_dir, 'test.raw')
    ImageManager.mk_file(file_path, 16 << 30)
    return file_path


@pytest.fixture(scope='session')
def tmp_dir(request):
    test_data_path = request.config.getoption("--test_data_path")
    if test_data_path == "":
        script_dir = os.path.dirname(os.path.realpath(__file__))
        test_data_path = os.path.join(script_dir, 'test_data')
        if not os.path.exists(test_data_path):
            os.mkdir(test_data_path)

    # unix socket paths are limited to circa 107 chars, so create short link
    # to workdir in /tmp
    tmp_root = os.path.join(tempfile.gettempdir(),
                            'test-libvhost-{}'.format(getuser()))
    os.makedirs(tmp_root, exist_ok=True)

    short_tmp_dir = tempfile.mkdtemp(dir=tmp_root)
    link_path = os.path.join(short_tmp_dir, 'data')
    os.symlink(src=test_data_path, dst=link_path)
    return link_path
