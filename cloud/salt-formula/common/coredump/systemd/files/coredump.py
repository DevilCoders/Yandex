import functools
import re
import time

from datetime import datetime, timedelta
from systemd import journal

DEFAULT_OFFSET = 60 * 60
DEFAULT_STATE = 1 # DEFAULT: Warning
MESSAGE_ID = "fc2e22bc6ee647b6b90729ab34a250b1"
COREPATTERN_SOURCE = "/proc/sys/kernel/core_pattern"
COREDUMP_PATTERN = "/lib/systemd/systemd-coredump"

try:
    import apt
except ImportError:
    RESOLVE_PKG = False
else:
    RESOLVE_PKG = True
    RESOLVE_PKG_CACHE = {}

class Mask:
    def __init__(self, state=DEFAULT_STATE, offset=DEFAULT_OFFSET, comm=None, exe=None, package=None):
        self.active = False
        self.offset = offset
        self.state = state
        if any([comm, exe, package]):
            self.active = True
            self.comm_pattern = re.compile(comm) if comm else None
            self.exe_pattern = re.compile(exe) if exe else None
            self.package_pattern = re.compile(package) if package else None

    def __check(self, pattern, coredump_attr):
        if pattern is None:
            return True
        return pattern.match(coredump_attr)

    def _check_comm(self, coredump):
        return self.__check(self.comm_pattern, coredump.get("COMM", ""))

    def _check_exe(self, coredump):
        return self.__check(self.exe_pattern, coredump.get("EXE", ""))

    def _check_package(self, coredump):
        return self.__check(self.package_pattern, coredump.get("PACKAGE", ""))

    def check(self, coredump):
        if self.active is False:
            return True
        return all(c(coredump) for c in (self._check_comm, self._check_exe, self._check_package))

def get_reader(offset):
    reader = journal.Reader()  # pylint: disable=E1101
    reader.seek_realtime(datetime.now() - timedelta(seconds=offset))
    reader.add_match(MESSAGE_ID=MESSAGE_ID)
    return reader

def check_core_pattern():
    with open(COREPATTERN_SOURCE, "r") as core_pattern:
       if COREDUMP_PATTERN in core_pattern.read():
           return True
    return False

def format_message(entry):
    formatted = {}
    for attr, value in entry.items():
        if attr.startswith("COREDUMP_"):
            formatted[attr[9:]] = value
        elif attr == "MESSAGE":
            formatted[attr] = value
    return formatted

def collect_coredumps(mask):
    dumps = []
    reader = get_reader(mask.offset)
    for entry in reader:
        formatted_entry = format_message(entry)
        if RESOLVE_PKG and mask.active and mask.package_pattern is not None:
            formatted_entry["PACKAGE"], formatted_entry["PACKAGE_VERSION"] = resolve_package(formatted_entry.get("EXE", ""))

        if not mask.check(formatted_entry):
            continue

        dumps.append(formatted_entry)
    reader.close()
    return dumps

def report_state(name, dumps, force_state):
    check_status = 0
    check_message = "OK"
    if not check_core_pattern():
        check_status = 2
        check_message = "You use not right core pattern for coredumps"
    elif dumps:
        check_status = force_state
        dumped_procs = []
        for dump in dumps:
            dumped_proc = dump.get("COMM", "unknown command")
            if RESOLVE_PKG:
                dumped_proc += " (from package: {}={})".format(*resolve_package(dump.get("EXE")))
            dumped_procs.append(dumped_proc)
        check_message = "{} fresh dump{}: {}".format(len(dumps), len(dumps) > 1 and "s" or "", ", ".join(dumped_procs))
    return format_output(name, check_status, check_message)

def format_output(name, status, message):
    check_type = "PASSIVE-CHECK"
    return "{}:{};{};{}".format(check_type, name, status, message)

def resolve_package(exe_path):
    if exe_path in RESOLVE_PKG_CACHE:
        return RESOLVE_PKG_CACHE[exe_path]
    apt_cache = apt.Cache()
    package_name = "unknown package"
    package_ver = "unknown version"
    try:
        for package_name in apt_cache.keys():
            if exe_path in apt_cache[package_name].installed_files:
                package = apt_cache[package_name]
                package_name = package.name
                package_ver = package._pkg.current_ver.ver_str
                break
    except:
        pass

    RESOLVE_PKG_CACHE[exe_path] = (package_name, package_ver)
    return (package_name, package_ver)

def check(name, mask):
    return report_state(name, collect_coredumps(mask), force_state=mask.state)
