#!/usr/bin/env python3

import os
import os.path
import shutil
import sys
import subprocess
import time
import signal
import argparse

## Common CI directories sructure
# libvhost-testing/
#     qemu-binary/
#         2.12.0-V/
#             usr/
#                 bin/
#                     qemu-system-x86_64
#     test.XXXXXX/
#         qemu/
#             x86_64-softmmu/
#                 qemu-system-x86_64 ->
#                     ../../../qemu-binary/2.12.0-V/usr/bin/qemu-system-x86_64
#         libvhost-server/
#             scripts/
#                 serve_continuous_test.py <- we are here
#             test/
#                 disks/
#                     prepare_images.sh
#                 tmp/
#                     seabios.log
#                     serial0.log
#                     qmp.sock
#                     vhost.sock
#                     vnc.sock
#                 pytest_host.sh
#                 aio-server
#         test-pid
#         test-start-date
#         test-status
#         common.log
#         qemu.log
#         vhost.log
#     latest -> test.XXXXXX
##

QEMU_VERSION_BASE = None

# symlink to test directory
TEST_SYMLINK = None

# envar name to set qemu binary to be used in the test
QEMU_BIN_ENVAR = "TEST_QEMU_BIN"

# Transform relative path inside "libvhost-testing" to absolute
def get_ci_path(*path):
    script_dir = os.path.dirname(os.path.realpath(__file__))
    ci_dir = os.path.normpath(os.path.join(script_dir, "../../../"))
    return os.path.normpath(os.path.join(ci_dir, *path))

# Transform relative path inside current test dir to absolute
def get_test_path(*path):
    script_dir = os.path.dirname(os.path.realpath(__file__))
    ci_dir = os.path.normpath(os.path.join(script_dir, "../../"))
    return os.path.normpath(os.path.join(ci_dir, *path))

# Returns path inside the last test run directory
def get_prev_test_path(*path):
    return get_ci_path(TEST_SYMLINK, *path)

# Get all QEMU subversions of base from latest to oldest.
# As example, [2.12.0-46, 2.12.0-45] are subversions of 2.12.0
def get_yc_qemu_versions():
    raw_list = subprocess.run(["apt-cache", "show", "qemu"],
        stdout=subprocess.PIPE).stdout.decode("utf-8")

    version_list = []
    for line in raw_list.split("\n"):
        if line.startswith("Version: 1:" + QEMU_VERSION_BASE + "-"):
            version_list.append(line[len("Version: 1:"):])

    # Place releases first, then PR versions
    def key_version(version_string):
        version_part = version_string[len(QEMU_VERSION_BASE + "-"):].split(".")
        if len(version_part) == 1:
            # release version
            return (1, int(version_part[0]), 0)
        else:
            # PR version
            return (0, int(version_part[0]), int(version_part[1]))

    version_list.sort(key=key_version, reverse=True)

    return version_list

# Download QEMU version if it missing, return path to qemu-system-x86_64 binary
def prepare_qemu_version(version):
    print("INFO: download QEMU {}".format(version))
    packages = [
        "qemu",
        "qemu-kvm",
        "qemu-system-common",
        "qemu-system-x86",
        "qemu-utils",
        "qemu-block-extra"
        ]

    for package in packages:
        subprocess.run([
            "apt",
            "download",
            "{}=1:{}".format(package, version)])
        subprocess.run([
            "dpkg-deb",
            "--extract",
            "{}_1%3a{}_amd64.deb".format(package, version),
            get_ci_path("qemu-binary", version)])
        subprocess.run([
            "rm",
            "{}_1%3a{}_amd64.deb".format(package, version)])

    ret_path = get_ci_path("qemu-binary", version, "usr", "bin",
                           "qemu-system-x86_64")
    assert(os.path.isfile(ret_path))

    return ret_path

# Prepare latest QEMU version and create link to it inside current test
def prepare_qemu():
    versions = get_yc_qemu_versions()
    assert(len(versions))

    if not os.path.isdir(get_ci_path("qemu-binary", versions[0])):
        qemu_binary = prepare_qemu_version(versions[0])
    else:
        qemu_binary = get_ci_path("qemu-binary", versions[0], "usr", "bin",
                                  "qemu-system-x86_64")
        assert(os.path.isfile(qemu_binary))
        print("INFO: use already downloaded QEMU {}".format(versions[0]))
    os.makedirs(get_test_path("qemu", "x86_64-softmmu"), exist_ok=True)
    if os.path.isfile(get_test_path("qemu", "x86_64-softmmu",
                                    "qemu-system-x86_64")):
        os.remove(get_test_path("qemu", "x86_64-softmmu", "qemu-system-x86_64"))
    os.symlink(qemu_binary,
               get_test_path("qemu", "x86_64-softmmu", "qemu-system-x86_64"))
    # set envar to be used in the test
    os.environ[QEMU_BIN_ENVAR] = qemu_binary

# Preparations for new test
def prepare_env():
    prepare_qemu()

def check_prev_test_dirs():
    if not os.path.islink(get_prev_test_path()) or \
        not os.path.isdir(get_prev_test_path()) or \
        not os.path.isfile(get_prev_test_path("test-start-date")) or \
        not os.path.isfile(get_prev_test_path("test-pid")) or \
        not os.path.isfile(get_prev_test_path("test-status")):
        return False
    else:
       return True

# Return status of latest test (or "failed" if there is no such)
def get_latest_test():
    if not check_prev_test_dirs():
        return "failed" # or not even started

    with open(get_prev_test_path("test-start-date")) as f:
        start_time = f.read()
    print("INFO: latest build was started at {}".format(start_time))

    with open(get_prev_test_path("test-status")) as f:
        latest_status = f.read()

    # no need to check if test not running (maybe it is used for debugging)
    if latest_status != "running":
        return latest_status # stopped or failed

    with open(get_prev_test_path("test-pid")) as f:
        test_pid = f.read()
    # check if process exist (if not, consider that test already failed)
    is_running = subprocess.call(["ps", "-p", test_pid]) == 0
    status = "running" if is_running else "failed"
    with open(get_prev_test_path("test-status"), "w") as f:
        f.write(status)

    return status

# Gracefully stop existing running test (used in case when no error are found)
def stop_previous_test():
    assert(check_prev_test_dirs())

    with open(get_prev_test_path("test-status")) as f:
        latest_status = f.read()
    assert(latest_status == "running")

    with open(get_prev_test_path("test-pid")) as f:
        test_pid = f.read()

    print("INFO: stopping PID {} using SIGINT".format(test_pid))
    subprocess.call(["kill", "-SIGINT", test_pid])
    time.sleep(3)

    if subprocess.call(["ps", "-p", test_pid]) == 0:
        print("WARN: can't stop gracefully, try to use SIGKILL")
        subprocess.call(["kill", "-SIGKILL", test_pid])
        time.sleep(3)

    if subprocess.call(["ps", "-p", test_pid]) == 0:
        print("ERROR: can't stop continuous test, mark as failed")
        status = "failed"
    else:
        status = "stopped"

    with open(get_prev_test_path("test-status"), "w") as f:
        f.write(status)

# Init some dirs if they're missing
def prepare_first_run():
    os.makedirs(get_ci_path("qemu-binary"), exist_ok=True)

# At this moment we only have folder with libvhost sources
def start_new_test():
    subprocess.call(["make",
                     "-C", get_test_path("libvhost-server", "test")])
    prepare_env()

    cmdline = [
        get_test_path("libvhost-server", "test", "pytest_host.sh"),
        get_test_path()]

    with open(get_test_path("test-stderr"), "w") as stderrf:
        with open(get_test_path("test-stdout"), "w") as stdoutf:
            test_pid = str(subprocess.Popen(cmdline,
                    shell=False,
                    close_fds=True,
                    stdin=subprocess.DEVNULL,
                    stdout=stdoutf,
                    stderr=stderrf).pid)

    with open(get_test_path("test-pid"), "w") as f:
        f.write(test_pid)

    with open(get_test_path("test-start-date"), "w") as f:
        f.write(time.ctime())
    time.sleep(3)

    is_running = subprocess.call(["ps", "-p", test_pid]) == 0
    status = "running" if is_running else "failed"
    with open(get_test_path("test-status"), "w") as f:
        f.write(status)

    prev_test_path = get_prev_test_path()

    if os.path.islink(prev_test_path):
        os.remove(prev_test_path)

    os.symlink(get_test_path(), get_ci_path(TEST_SYMLINK))


TEST_STALE_TTL_SECONDS = (5 * 24 * 60 * 60)  # 5 days


def test_not_running_and_stale(path):
    if os.path.exists(os.path.join(path, "lock")):  # ignore manual locks
        print("INFO: skip test {} due to lock".format(path))
        return False

    now = time.time()
    mtime = None
    test_status_file_path = os.path.join(path, "test-status")
    if os.path.isfile(test_status_file_path):
        with open(test_status_file_path) as f:
            test_status = f.read()
            if test_status == "running":  # ignore running tests
                print("INFO: skip test {} due to it's running".format(path))
                return False

            stat = os.fstat(f.fileno())
            mtime = stat.st_mtime
    else:
        mtime = max(os.stat(root).st_mtime for root, _, _ in os.walk(path))

    print("INFO: test path {} mtime {} now {}".format(path, mtime, now))

    if (mtime is not None) and (now > mtime):
        if ((now - mtime) >= TEST_STALE_TTL_SECONDS):
            return True

    return False


def get_pid_from_file(file_path, comm):
    # pid files are cleaned up on pytest shutdown, so this is not an error
    if not os.path.exists(file_path):
        return 0

    try:
        with open(file_path, "r") as f:
            pid = int(f.read())
        with open("/proc/{}/comm".format(pid)) as f:
            pid_comm = f.read().strip("\n")
        if pid_comm != comm:
            message = "pid mismath: pid {pid} refers to {pid_comm}, not {comm}"
            raise RuntimeError(message.format(pid, pid_comm, comm))
    except Exception as e:
        print("ERROR: pid file {} exception {}".format(file_path, str(e)))
        return 0

    return pid


def get_test_pids(test_path):
    pids = []
    comm_by_pid_file = {"qemu-pid": "qemu-system-x86",
                        "aio-server-pid": "aio-server"}
    for pid_file in comm_by_pid_file.keys():
        pid = get_pid_from_file(os.path.join(test_path, pid_file),
                                comm_by_pid_file[pid_file])
        if pid > 0:
            pids.append(pid)

    return pids


def remove_stale_test(test_path):
    print("INFO: remove_stale_test({}) begin".format(test_path))

    pids = get_test_pids(test_path)
    if len(pids) != 0:
        for sig in [signal.SIGTERM, signal.SIGKILL]:
            for pid in pids:
                print("INFO: try killing "
                      "test {} pid {} "
                      "by signal {}".format(test_path, pid, sig))
                os.kill(pid, sig)

            print("INFO: wait test {} pids exits".format(test_path))
            time.sleep(30)

            pids = get_test_pids(test_path)
            if len(pids) == 0:
                print("INFO: test {} no pid found".format(test_path))
                break

    print("INFO: test {} rmtree".format(test_path))
    shutil.rmtree(test_path)


def remove_stale_tests():
    try:
        tests_root = get_ci_path()
        print("INFO: remove_stale_tests() begin tests_root {}", tests_root)

        for file in os.listdir(tests_root):
            test_path = os.path.join(tests_root, file)
            if os.path.isdir(test_path) and file.startswith("test"):
                if test_not_running_and_stale(test_path):
                    remove_stale_test(test_path)

        print("INFO: remove_stale_tests() done")
    except Exception as e:
        print("ERROR: remove_stale_tests() exception {}".format(str(e)))


# Must be runned from "libvhost-testing/test.XXXXXX/libvhost-server/scripts/"
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("qemu_version_base", help="qemu version base")

    args = parser.parse_args()

    QEMU_VERSION_BASE = args.qemu_version_base
    TEST_SYMLINK = "latest_{}".format(QEMU_VERSION_BASE)

    print("INFO: CI dir \"{}\"".format(get_ci_path()))
    print("INFO: test dir \"{}\"".format(get_test_path()))
    if os.path.exists(get_prev_test_path()):
        prev_test_dir = os.readlink(get_prev_test_path())
        print("INFO: previous test dir is \"{}\"".format(prev_test_dir))

    # Check and create folders and links if they are missing
    prepare_first_run()

    previous_status = get_latest_test()

    if previous_status == "running":
        print("INFO: test still run; gracefully stop it")
        stop_previous_test()
    elif "failed":
        print("WARN: previous test already dead")

    # Start new test; if previous was failed, use old directory
    print("INFO: starting new test")
    start_new_test()

    time.sleep(10)
    test_status = get_latest_test()

    if previous_status == "failed":
        print("ERROR: previous test was failed")

    if test_status == "running":
        print("INFO: new test successfully started")
    else:
        print("ERROR: new test failed after start")

    exit_code = 0
    if previous_status == "failed" or test_status == "failed":
        exit_code = -2

    remove_stale_tests()

    sys.exit(exit_code)
