# from __future__ import _print_function
# from __future__ import unicode_literals

import errno
import json
import os
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import urllib2
import uuid
import pprint

import pytest
import requests
import copy

import config
from core.svnapi import SvnRepository
from gaux.aux_utils import mkdirp

_POLLING_INTERVAL = 0.01


# ------------------------------------------------------------------------------
#   CMD LINE OPTIONS

def pytest_addoption(parser):
    parser.addoption("--db-type", action="store", default="unstable",
                     help="database type: unstable or tags")
    parser.addoption("--host", type=str, default="localhost",
                     help="optional. Existing backend host, localhost by default")
    parser.addoption("--port", type=int, default=None,
                     help="optional. Existing backend port")
    parser.addoption("--do-traverse", default=False, action="store_true",
                     help="optional. Run traverse test")


# ------------------------------------------------------------------------------

class _GencfgEnvironment(object):
    """Mocks gencfg environment (repositories) where it's safe to commit and push.

    Properties:
    * path - main repo path
    * db_path - DB path
    """

    def __init__(self, request):
        self.__temp_dir = None
        request.addfinalizer(self.close)

        try:
            self.__init()
        except:
            self.close()
            raise

    def close(self):
        if self.__temp_dir is not None:
            shutil.rmtree(self.__temp_dir)
            self.__temp_dir = None

    def __init(self):
        print('Start building gencfg env...')

        self.__temp_dir = tempfile.mkdtemp(dir="/var/tmp")
        print ("Created temporary dir <%s>" % self.__temp_dir)

        before_time = time.time()
        self.__prepare_main_repo()
        print("__prepare_main_repo: %s" % (time.time() - before_time))

        before_time = time.time()
        self.__check_test_environment()
        print("__check_test_environment: %s" % (time.time() - before_time))

    def __prepare_main_repo(self):
        """Create a safe copy of main repository."""

        git_files = set()
        repo_src_path = os.getcwd()

        for file_path in SvnRepository(repo_src_path).command(["ls", "-R"]).stdout.strip().split("\n"):
            for exclude in ("validator", "usageanalyzer", "db_new"):
                if file_path == exclude or file_path.startswith(exclude + "/"):
                    break
            else:
                git_files.add(file_path)
                while os.path.sep in file_path:
                    file_path = os.path.dirname(file_path)
                    git_files.add(file_path)

        def ignore_func(path, names):
            ignore_names = set()

            for name in names:
                file_path = os.path.join(path, name)
                assert file_path.startswith(repo_src_path + "/")
                file_path = file_path[len(repo_src_path) + 1:]

                if file_path not in git_files:
                    ignore_names.add(name)

            return ignore_names

        self.__main_repo_path = os.path.join(self.__temp_dir, "main_repo")
        shutil.copytree(repo_src_path, self.__main_repo_path, symlinks=False, ignore=ignore_func)
        self.path = self.__main_repo_path
        self.db_path = os.path.join(self.__main_repo_path, 'db')
        link_path = os.path.join(self.path, config.VENV_REL_PATH)
        mkdirp(os.path.dirname(link_path))
        os.symlink(config.VENV_DIR, link_path)

    def __check_test_environment(self):
        """We must sure that out test environment is properly set up.

        In other case we can accidentally run code from the real repository - not from the mocked one.
        In the worst case we will commit changes to the real repository instead of the mocked ones.
        """

        env = _make_local_env()
        process = subprocess.Popen(["tests/check_environment"], cwd=self.path,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True, env=env)
        stdout, stderr = process.communicate()
        assert stderr == "" and process.returncode == 0

        db_module_path, db_path = stdout.strip().split("\n")

        is_db = os.path.samefile(db_path, self.db_path)
        assert is_db

        db_py_path = os.path.join(self.path, "core", "db.py")
        db_pyc_path = db_py_path + "c"
        is_db_py = os.path.samefile(db_module_path, db_py_path)
        is_db_pyc = os.path.exists(db_pyc_path) and os.path.samefile(db_module_path, db_pyc_path)

        assert is_db_py or is_db_pyc
        assert os.path.samefile(db_path, os.path.join(self.path, "db"))

    @staticmethod
    def __get_repo_logs(repo):
        commits = repo.command(["log", "--all", "--format=%s"]).stdout.strip().split("\n")
        commits.reverse()
        return commits


class _ServiceApi(object):
    def __init__(self, host, port, db_type, backend_type):
        self.__host = host
        self.__port = port
        self.__db_type = db_type
        self.__backend_type = backend_type

    def get_host(self):
        return self.__host

    def get_port(self):
        return self.__port

    def get_db_type(self):
        return self.__db_type

    def set_db_type(self, db_type):
        self.__db_type = db_type

    def get_backend_type(self):
        return self.__backend_type

    def get(self, method, **kwargs):
        return self.__call("GET", method, **kwargs)

    def post(self, method, request, **kwargs):
        return self.__call("POST", method, request=request, **kwargs)

    def __call(self, http_method, api_method, params=None, request=None, ret_raw_result=False, raise_bad_statuses=True):
        if api_method.startswith('/unstable/'):
            if self.__db_type == "tags":
                api_method = api_method.replace('/unstable/', '/tags/recent/')

        url = "http://{host}:{port}{api_method}".format(host=self.__host, port=self.__port, api_method=api_method)

        kwargs = {}
        if request is not None:
            kwargs["headers"] = {"Content-Type": "application/json"}
            kwargs["data"] = json.dumps(request)

        response = requests.request(http_method, url, params=params, **kwargs)
        if response.status_code != requests.codes.ok and raise_bad_statuses:
            raise Exception(response.reason + ":\n" + response.text)
        if ret_raw_result:
            return response
        else:
            return response.json()

    def check_response_status(self, response, ok=True):
        if response.get("operation success") is ok:
            return response

        raise Exception("Operation failed: \n" + pprint.pformat(response))


class GencfgSocketErr(Exception):
    pass


class _UserServiceInstance(object):
    def __init__(self, backend_type, host, port, db_type):
        assert (host is not None or port is not None)
        assert (type(host) == str)
        assert (type(port) == int)
        self.__host = host
        self.__port = port
        self.__backend_type = backend_type
        self.api = _ServiceApi(self.__host, self.__port, None, self.__backend_type)

        result = self.api.get("/testing_info")

        actual_db_type = result["db_type"]
        if db_type != actual_db_type:
            raise Exception("Backend db_type mismatch, backend db_type is \"%s\", but \"%s\" is required" % \
                             (actual_db_type, db_type))
        self.api.set_db_type(actual_db_type)

        commit_enabled = result["commit_enabled"]
        if commit_enabled and db_type == "unstable":
            raise Exception("Cannot run tests on backend without --no-commit option, as test commits will be pushed to real db")

        actual_backend_type = result["backend_type"]
        if backend_type != actual_backend_type:
            raise Exception("Backend type mismatch, backend is \"%s\", but \"%s\" is required" % \
                             (actual_backend_type, backend_type))

        # now get db_path
        result = self.api.get("/unstable/testing_info")
        self.db_path = result["db_path"]
        self.__backend_type = backend_type


class _TestingServiceInstance(_GencfgEnvironment):
    def __init__(self, executable, db_type, *args, **kwargs):
        self.__pid = None
        self.__host = "localhost"
        self.__executable = executable
        self.__backend_type = os.path.basename(os.path.dirname(os.path.abspath(executable)))
        self.__db_type = db_type
        super(_TestingServiceInstance, self).__init__(*args, **kwargs)

        try:
            self.__init()
            self.api = _ServiceApi(self.__host, self.__port, self.__db_type, self.__backend_type)
        except:
            self.close()
            raise

    def close(self):
        try:
            if self.__pid is not None:
                # do we need it?
                try:
                    conn = urllib2.urlopen("http://localhost:%s/setup?action=die" % self.__port, data="{}")
                    conn.read()
                except:
                    pass

                _kill_process_group(self.__pid)
                self.__pid = None

                try:
                    with open(os.path.join(self.path, "logs", "gencfgd-{}.txt".format(self.__port))) as log_file:
                        logs = log_file.read()
                except EnvironmentError as e:
                    if e.errno != errno.ENOENT:
                        raise e
                else:
                    print("Server logs:\n" + logs)
        finally:
            super(_TestingServiceInstance, self).close()

    def __init(self):
        print('Starting service...')
        success = False

        # try to do few attempts, because sometimes we can have a race condition when finding unused port
        for attempt in range(3):
            print('Try to start service, %s attempt' % (attempt + 1))
            self.__port = self.__get_free_port()
            self.__uuid = str(uuid.uuid1())
            start_time = time.time()
            with open(os.devnull) as devnull:
                env = _make_local_env()
                process = subprocess.Popen(
                    [self.__executable, "-p", str(self.__port), "--no-auth", "--uuid", self.__uuid, "--db",
                     self.__db_type, "--no-commit"],
                    cwd=self.path, preexec_fn=os.setpgrp, stdin=devnull, close_fds=True, env=env)
            self.__pid = process.pid
            try:
                self.__wait_for_start(process)
                success = True
                break
            except GencfgSocketErr:
                pass
            time.sleep(1)

        if not success:
            # we have some attempts and failed with socket error
            raise Exception('Made some attempts to run, but failed with socket error')

        print('Service started in %s seconds' % (time.time() - start_time))

    def __get_free_port(self):
        # This algorithm creates race conditions or some other bad problems
        #
        # with contextlib.closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        #    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        #    sock.bind((self.__host, 0))
        #    return sock.getsockname()[1]

        # will generate some random port in range 10k to 20k
        max_delta = 2
        assert (self.__executable in ["wbe/main.py", "api/main.py"])
        delta = int(self.__executable == "wbe/main.py")
        return 10000 + (os.getpid() * max_delta + delta) % 10000

    def __wait_for_start(self, process):
        start_timeout = time.time() + 60

        is_socket_opened = False
        while process.poll() is None and time.time() < start_timeout:
            try:
                socket.create_connection((self.__host, self.__port)).close()
                is_socket_opened = True
                break
            except socket.error:
                time.sleep(_POLLING_INTERVAL)

        retcode = process.poll()
        if retcode is not None:
            if retcode == config.SERVICE_EXIT_CODE_SOCKET_ERR:
                # port race condition
                raise GencfgSocketErr()
            raise Exception("Web backend has crashed. See its output for details.")

        if not is_socket_opened:
            raise Exception("Unable to connect to Web backend")

        is_backend_on_port = self.__check_identity(self.__port, self.__uuid)
        if not is_backend_on_port:
            # port race condition
            raise GencfgSocketErr()

    def __check_identity(self, port, process_uuid):
        del port
        # we check that it is our process on specific port
        is_valid_response = True
        try:
            conn = urllib2.urlopen("http://localhost:%s/setup" % self.__port)
            is_valid_response = is_valid_response and conn.getcode() == 200
            if is_valid_response:
                data = conn.read()
                is_valid_response = json.loads(data)["uuid"] == process_uuid
        except Exception:
            is_valid_response = False
        return is_valid_response


@pytest.fixture
def temp_dir(request):
    path = tempfile.mkdtemp(dir="/var/tmp")
    request.addfinalizer(lambda: shutil.rmtree(path))
    return path


def pytest_runtest_setup(item):
    if 'unstable_only' in item.keywords and item.config.getoption("--db-type") != "unstable":
        with open("/var/tmp/1.log", "w") as f:
            print >> f, "OOOOO", item.config.getoption("--db-type")
            print >> f, item.config
            print >> f, item
            print >> f, item.keywords

        pytest.skip("applicable only for unstable")
    # traversal will be parallelized with Makefile, not xdist
    # as xdist balances tests between threads in not efficient and not predictable way
    if 'is_traverse_test' in item.keywords and \
            (item.config.getoption("--db-type") != "tags" or not item.config.getoption("--do-traverse")):
        pytest.skip("traverse tests are applicable only for --db-type = tags and --do-traverse options")


@pytest.fixture
def gencfg_env(request):
    return _GencfgEnvironment(request)


@pytest.fixture(scope="session")
def wbe(request):
    db_type = request.config.getoption("--db-type")
    backend_host = request.config.getoption("--host")
    backend_port = request.config.getoption("--port")

    if backend_host is not None and backend_port is not None:
        return _UserServiceInstance("wbe", backend_host, backend_port, db_type)
    else:
        return _TestingServiceInstance("wbe/main.py", db_type, request)


@pytest.fixture(scope="session")
def api(request):
    db_type = request.config.getoption("--db-type")
    backend_host = request.config.getoption("--host")
    backend_port = request.config.getoption("--port")

    if backend_host is not None and backend_port is not None:
        return _UserServiceInstance("api", backend_host, backend_port, db_type)
    else:
        return _TestingServiceInstance("api/main.py", db_type, request)


def _kill_process_group(pid):
    terminate_timeout = time.time() + 5

    while True:
        try:
            os.kill(pid, signal.SIGTERM)
            if os.waitpid(pid, os.WNOHANG)[0] == pid:
                break
        except EnvironmentError as e:
            if e.errno in (errno.ESRCH, errno.ECHILD):
                break
            else:
                raise

        if time.time() >= terminate_timeout:
            break

        time.sleep(_POLLING_INTERVAL)

    try:
        os.kill(-pid, signal.SIGKILL)
    except EnvironmentError as e:
        if e.errno != errno.ESRCH:
            raise


def _make_local_env():
    # py.test xdist patches PYTHONPATH!!!
    # it is fucking not what we need
    result = copy.deepcopy(os.environ)
    if 'PYTHONPATH' in result:
        del result['PYTHONPATH']
    return result


@pytest.fixture(scope="session")
def curdb():
    import gencfg
    from core.db import CURDB
    return CURDB


@pytest.fixture(scope="session")
def testdb_path():
    return os.path.join(os.path.dirname(os.path.dirname(__file__)), "testdb")
